#!/usr/bin/env python3
# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Run each case of a benchmark suite as its own Google Benchmark subprocess with a hard timeout."""

import argparse
import concurrent.futures
import io
import json
import os
import pathlib
import platform
import queue
import re
import subprocess
import sys
import time
from dataclasses import dataclass
from typing import Any, TextIO, TypedDict, cast

from report import build_summary, print_summary
from schema import require_mapping, validate_suite


class CommandResult(TypedDict):
    """Outcome of one benchmark subprocess; status is passed, failed or timed_out."""

    command: list[str]
    status: str
    exit_code: int | None
    duration_seconds: float
    stdout: str


@dataclass(frozen=True)
class BenchmarkOptions:
    """How every case's Google Benchmark subprocess runs."""

    suite_json: pathlib.Path
    benchmark_args: list[str]
    min_time: str
    repetitions: int | None
    report_aggregates_only: bool
    timeout: float
    jobs: int


def run_text(command: list[str], cwd: pathlib.Path | None = None) -> str | None:
    """The command's stripped stdout, or None if it exits nonzero."""
    result = subprocess.run(command, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, check=False)
    if result.returncode != 0:
        return None
    return result.stdout.strip()


def find_cmake_cache(path: pathlib.Path) -> pathlib.Path | None:
    """The CMakeCache.txt in ``path`` or its nearest ancestor that has one."""
    for candidate in [path, *path.parents]:
        cache = candidate / "CMakeCache.txt"
        if cache.exists():
            return cache
    return None


def read_cmake_cache(cache_path: pathlib.Path | None) -> dict[str, str]:
    """Build type, compiler and flags from the CMake cache, plus the cache's path; empty without a cache."""
    if cache_path is None:
        return {}

    result: dict[str, str] = {}
    wanted = {
        "CMAKE_BUILD_TYPE",
        "CMAKE_CXX_COMPILER",
        "CMAKE_CXX_COMPILER_ID",
        "CMAKE_CXX_COMPILER_VERSION",
        "CMAKE_CXX_FLAGS",
    }

    for line in cache_path.read_text(errors="replace").splitlines():
        if line.startswith(("//", "#")) or "=" not in line:
            continue
        key_with_type, value = line.split("=", 1)
        key = key_with_type.split(":", 1)[0]
        if key in wanted:
            result[key] = value

    result["CMakeCache"] = str(cache_path)
    return result


def read_cmake_compiler_metadata(cache_path: pathlib.Path | None) -> dict[str, str]:
    """Compiler id and version from CMake's detected-compiler file next to the cache; empty if there is none."""
    if cache_path is None:
        return {}

    cmake_files = cache_path.parent / "CMakeFiles"
    compiler_files = list(cmake_files.glob("*/CMakeCXXCompiler.cmake"))
    if not compiler_files:
        return {}

    result: dict[str, str] = {}
    wanted = {
        "CMAKE_CXX_COMPILER_ID",
        "CMAKE_CXX_COMPILER_VERSION",
    }
    pattern = re.compile(r'set\((?P<key>[A-Za-z0-9_]+)\s+"(?P<value>.*)"\)')

    for line in compiler_files[0].read_text(errors="replace").splitlines():
        match = pattern.match(line)
        if match and match.group("key") in wanted:
            result[match.group("key")] = match.group("value")

    return result


def collect_metadata(
    args: argparse.Namespace, executable: pathlib.Path, test_dir: pathlib.Path, output_dir: pathlib.Path
) -> dict[str, Any]:
    """Git state, build configuration, host, and runner options recorded in the summary."""
    cache_path = find_cmake_cache(test_dir)
    return {
        "git": {
            "commit": run_text(["git", "rev-parse", "HEAD"]),
            "branch": run_text(["git", "rev-parse", "--abbrev-ref", "HEAD"]),
            "dirty": bool(run_text(["git", "status", "--porcelain"])),
        },
        "build": {
            "executable": str(executable),
            "test_dir": str(test_dir),
            **read_cmake_cache(cache_path),
            **read_cmake_compiler_metadata(cache_path),
        },
        "host": {
            "hostname": platform.node(),
            "platform": platform.platform(),
            "cpu_count": os.cpu_count(),
        },
        "runner": {
            "command": sys.argv,
            "suite_json": str(args.suite_json),
            "suite_prefix": args.suite_prefix,
            "output_dir": str(output_dir),
            "benchmark_min_time": args.benchmark_min_time,
            "benchmark_repetitions": args.benchmark_repetitions,
            "benchmark_report_aggregates_only": args.benchmark_report_aggregates_only,
            "benchmark_timeout_seconds": args.benchmark_timeout,
            "benchmark_args": args.benchmark_arg,
        },
    }


def load_suite(suite_json: pathlib.Path) -> dict[str, Any]:
    """Read and validate a benchmark suite JSON file."""
    suite = json.loads(suite_json.read_text())
    validate_suite(suite)
    return suite


def suite_prefix(suite: dict[str, Any]) -> str:
    """The suite's path prefix variable, BENCHMARKS_DIR by default."""
    return suite.get("prefix", "BENCHMARKS_DIR")


def normalize_stdout(stdout: str | bytes | None) -> str:
    """Subprocess output as text; a timed-out process may leave bytes or nothing."""
    if stdout is None:
        return ""
    if isinstance(stdout, bytes):
        return stdout.decode(errors="replace")
    return stdout


def load_cases(suite: dict[str, Any]) -> list[dict[str, Any]]:
    """One case per domain task, named domain/task."""
    cases: list[dict[str, Any]] = []

    for domain_name, domain_config in suite["domains"].items():
        for task_name, task_file in domain_config["tasks"].items():
            cases.append(
                {
                    "run_name": f"{domain_name}/{task_name}",
                    "domain": domain_name,
                    "task": task_name,
                    "domain_file": domain_config["domain_file"],
                    "problem_file": task_file,
                }
            )

    return cases


def build_benchmark_command(
    benchmark_executable: pathlib.Path, run_name: str, result_file: pathlib.Path | None, options: BenchmarkOptions
) -> list[str]:
    """The command running only ``run_name``'s benchmarks, writing JSON results to ``result_file`` if given."""
    # Google Benchmark uses POSIX regex on Unix; Python's re.escape also escapes
    # ordinary characters such as '-' that POSIX rejects outside character sets.
    escaped_name = "".join("\\" + char if char in r"\.^$|?*+()[]{}" else char for char in run_name)
    command = [
        str(benchmark_executable),
        f"--suite-json={options.suite_json.resolve()}",
        *options.benchmark_args,
        f"--benchmark_filter=^{escaped_name}/.*",
        f"--benchmark_min_time={options.min_time}",
        "--benchmark_format=json",
    ]
    if result_file is not None:
        command.extend(
            [
                f"--benchmark_out={result_file}",
                "--benchmark_out_format=json",
            ]
        )
    if options.repetitions is not None:
        command.append(f"--benchmark_repetitions={options.repetitions}")
    if options.report_aggregates_only:
        command.append("--benchmark_report_aggregates_only=true")
    return command


def run_command(command: list[str], timeout: float, cpu: int | None = None) -> CommandResult:
    """Run the command with a wall-clock timeout, capturing stdout and stderr together; on ``cpu`` alone if given."""
    started = time.monotonic()
    with subprocess.Popen(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as process:
        if cpu is not None:
            os.sched_setaffinity(process.pid, {cpu})
        try:
            output, _ = process.communicate(timeout=timeout)
            exit_code = process.returncode
            status = "passed" if exit_code == 0 else "failed"
        except subprocess.TimeoutExpired:
            process.kill()
            output, _ = process.communicate()
            exit_code = None
            status = "timed_out"
    duration = time.monotonic() - started
    stdout = normalize_stdout(output)

    return {
        "command": command,
        "status": status,
        "exit_code": exit_code,
        "duration_seconds": duration,
        "stdout": stdout,
    }


def write_log_entry(log_file: TextIO, run_name: str, phase: str, result: CommandResult) -> None:
    """Append the command's output to the log under a header naming the case, phase and status."""
    log_file.write(f"===== {run_name} ({phase}: {result['status']}) =====\n")
    log_file.write(result["stdout"])
    if result["stdout"] and not result["stdout"].endswith("\n"):
        log_file.write("\n")


def load_benchmark_records(path: pathlib.Path) -> list[dict[str, Any]]:
    """The benchmark records of a Google Benchmark JSON file; empty if its benchmarks are not a list.

    Raises:
        OSError: If the file cannot be read.
        ValueError: If the file is not JSON.
        KeyError: If the output has no benchmarks.
        TypeError: If the output or a record is not a JSON object.
    """
    data = require_mapping(json.loads(path.read_text()), "benchmark output")
    raw_benchmarks: object = data["benchmarks"]
    if not isinstance(raw_benchmarks, list):
        return []
    return [require_mapping(item, "benchmark") for item in cast("list[object]", raw_benchmarks)]


def benchmark_records_error(benchmarks: list[dict[str, Any]], run_name: str) -> str | None:
    """Why the records are not a valid, error-free result of ``run_name``, or None if they are."""
    if not benchmarks or not all(
        isinstance(item.get("name"), str) and item["name"].startswith(f"{run_name}/") for item in benchmarks
    ):
        return "Missing or mismatched benchmark results."
    errors = [item.get("error_message", "Benchmark error.") for item in benchmarks if item.get("error_occurred")]
    if errors:
        return "; ".join(errors)
    if any(not isinstance(item.get(key), (int, float)) for item in benchmarks for key in ("real_time", "cpu_time")):
        return "Missing or invalid benchmark timings."
    if any(item.get("time_unit") not in {"ns", "us", "ms", "s"} for item in benchmarks):
        return "Missing or invalid benchmark time unit."
    return None


def validate_benchmark_result(path: pathlib.Path, run_name: str) -> str | None:
    """Why the benchmark JSON at ``path`` is not a valid, error-free result of ``run_name``, or None if it is."""
    try:
        return benchmark_records_error(load_benchmark_records(path), run_name)
    except (OSError, ValueError, KeyError, TypeError):
        return "Missing or invalid benchmark JSON."


def run_case(
    benchmark_executable: pathlib.Path,
    output_dir: pathlib.Path,
    case: dict[str, Any],
    options: BenchmarkOptions,
    cpu: int | None,
) -> tuple[dict[str, Any] | None, dict[str, Any] | None, str]:
    """Run one case, on ``cpu`` alone if given, recording its status in the case.

    Returns its result or its failure, and its log entry.
    """
    timeout = options.timeout
    run_name = case["run_name"]
    result_file = output_dir / "benchmark-results" / f"{run_name}.json"
    temp_result_file = result_file.with_name(f"{result_file.name}.tmp")
    result_file.parent.mkdir(parents=True, exist_ok=True)
    result_file.unlink(missing_ok=True)
    temp_result_file.unlink(missing_ok=True)

    command = build_benchmark_command(benchmark_executable, run_name, temp_result_file, options)
    result = run_command(command, timeout, cpu)
    if result["status"] == "passed":
        error = validate_benchmark_result(temp_result_file, run_name)
        if error:
            result["status"] = "failed"
            result["stdout"] += f"\n{error}\n"
    log = io.StringIO()
    write_log_entry(log, run_name, "benchmark", result)

    success: dict[str, Any] | None = None
    failure: dict[str, Any] | None = None
    if result["status"] == "passed":
        temp_result_file.replace(result_file)
        success = {
            "run_name": run_name,
            "command": command,
            "status": result["status"],
            "exit_code": result["exit_code"],
            "result_file": str(result_file),
            "duration_seconds": result["duration_seconds"],
        }
        case["benchmark_result_file"] = str(result_file)
    else:
        temp_result_file.unlink(missing_ok=True)
        reason = "benchmark_timed_out" if result["status"] == "timed_out" else "benchmark_failed"
        failure = {
            "run_name": run_name,
            "command": command,
            "status": result["status"],
            "exit_code": result["exit_code"],
            "duration_seconds": result["duration_seconds"],
            "reason": reason,
        }
        if result["status"] == "timed_out":
            failure["timeout_seconds"] = timeout
        case["benchmark_failure_reason"] = reason
        if result["status"] == "timed_out":
            case["benchmark_timeout_seconds"] = timeout

    case["status"] = result["status"]
    case["benchmark_status"] = result["status"]
    case["benchmark_duration_seconds"] = result["duration_seconds"]
    case["benchmark_exit_code"] = result["exit_code"]
    return success, failure, log.getvalue()


def run_benchmarks(
    benchmark_executable: pathlib.Path, output_dir: pathlib.Path, cases: list[dict[str, Any]], options: BenchmarkOptions
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Run every case, ``options.jobs`` at a time, each on a CPU of its own.

    One at a time, on every CPU, for cases that run threads of their own. Logs their output in suite order, records each
    status in its case, and returns the results and failures.
    """
    cpus: queue.Queue[int] = queue.Queue()
    for cpu in sorted(os.sched_getaffinity(0))[: options.jobs]:
        cpus.put(cpu)

    def on_free_cpu(case: dict[str, Any]) -> tuple[dict[str, Any] | None, dict[str, Any] | None, str]:
        cpu = cpus.get()
        try:
            return run_case(benchmark_executable, output_dir, case, options, cpu if options.jobs > 1 else None)
        finally:
            cpus.put(cpu)

    with concurrent.futures.ThreadPoolExecutor(max_workers=cpus.qsize()) as pool:
        outcomes = list(pool.map(on_free_cpu, cases))

    results = [success for success, _, _ in outcomes if success is not None]
    failures = [failure for _, failure, _ in outcomes if failure is not None]
    with (output_dir / "benchmark.log").open("w") as benchmark_log:
        for _, _, log in outcomes:
            benchmark_log.write(log)
    return results, failures


def main() -> int:
    """Run the suite's selected cases, write summary.json, print the summary, and return its exit code."""
    parser = argparse.ArgumentParser(description="Run each benchmark case with a hard wall-clock timeout.")
    parser.add_argument("--executable", type=pathlib.Path, required=True, help="Benchmark executable to run.")
    parser.add_argument(
        "--output-dir",
        required=True,
        help="Directory for benchmark.log, summary.json, and benchmark result JSON files.",
    )
    parser.add_argument(
        "--suite-json",
        type=pathlib.Path,
        required=True,
        help="Benchmark suite JSON with domains, domain files, and task files.",
    )
    parser.add_argument("--benchmark-min-time", default="0.01s", help="Google Benchmark --benchmark_min_time value.")
    parser.add_argument("--benchmark-repetitions", type=int, help="Google Benchmark --benchmark_repetitions value.")
    parser.add_argument(
        "--benchmark-arg",
        action="append",
        default=[],
        help="Extra executable option; repeat as --benchmark-arg=--cores=1,16.",
    )
    parser.add_argument("--case-filter", help="Regular expression selecting domain/task names from the suite.")
    parser.add_argument(
        "--benchmark-timeout",
        type=float,
        default=10.0,
        help="Hard wall-clock timeout in seconds for each per-case Google Benchmark subprocess.",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=24,
        help="Cases run at a time, each on a CPU of its own; 1 runs them one at a time on every CPU (threaded cases).",
    )
    parser.add_argument(
        "--benchmark-report-aggregates-only",
        action="store_true",
        help="Forward --benchmark_report_aggregates_only=true to Google Benchmark.",
    )
    args = parser.parse_args()
    if args.benchmark_timeout <= 0:
        parser.error("--benchmark-timeout must be greater than 0.")
    if args.jobs <= 0:
        parser.error("--jobs must be greater than 0.")

    output_dir = pathlib.Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    executable = args.executable.resolve()
    test_dir = executable.parent
    summary_file = output_dir / "summary.json"

    suite = load_suite(args.suite_json)
    cases = load_cases(suite)
    if args.case_filter:
        cases = [case for case in cases if re.search(args.case_filter, case["run_name"])]
    if not cases:
        parser.error("No cases matched --case-filter.")
    args.suite_prefix = suite_prefix(suite)
    metadata = collect_metadata(args, executable, test_dir, output_dir)

    options = BenchmarkOptions(
        suite_json=args.suite_json,
        benchmark_args=args.benchmark_arg,
        min_time=args.benchmark_min_time,
        repetitions=args.benchmark_repetitions,
        report_aggregates_only=args.benchmark_report_aggregates_only,
        timeout=args.benchmark_timeout,
        jobs=args.jobs,
    )
    benchmark_results, benchmark_failures = run_benchmarks(executable, output_dir, cases, options)
    summary = build_summary(suite, cases, metadata, benchmark_results, benchmark_failures)

    rendered_summary = json.dumps(summary, indent=2)
    summary_file.write_text(rendered_summary + "\n")
    print_summary(summary)

    return summary["exit_code"]


if __name__ == "__main__":
    sys.exit(main())
