#!/usr/bin/env python3
import argparse
import json
import os
import pathlib
import platform
import re
import subprocess
import sys
import time
from typing import Any, TextIO, TypedDict, cast

from report import build_summary, print_summary
from schema import require_mapping, validate_suite


class CommandResult(TypedDict):
    command: list[str]
    status: str
    exit_code: int | None
    duration_seconds: float
    stdout: str


def run_text(command: list[str], cwd: pathlib.Path | None = None) -> str | None:
    result = subprocess.run(command, cwd=cwd, text=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    if result.returncode != 0:
        return None
    return result.stdout.strip()


def find_cmake_cache(path: pathlib.Path) -> pathlib.Path | None:
    for candidate in [path, *path.parents]:
        cache = candidate / "CMakeCache.txt"
        if cache.exists():
            return cache
    return None


def read_cmake_cache(cache_path: pathlib.Path | None) -> dict[str, str]:
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
        if line.startswith("//") or line.startswith("#") or "=" not in line:
            continue
        key_with_type, value = line.split("=", 1)
        key = key_with_type.split(":", 1)[0]
        if key in wanted:
            result[key] = value

    result["CMakeCache"] = str(cache_path)
    return result


def read_cmake_compiler_metadata(cache_path: pathlib.Path | None) -> dict[str, str]:
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


def collect_metadata(args: argparse.Namespace, executable: pathlib.Path, test_dir: pathlib.Path, output_dir: pathlib.Path) -> dict[str, Any]:
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
    suite = json.loads(suite_json.read_text())
    validate_suite(suite)
    return suite


def suite_prefix(suite: dict[str, Any]) -> str:
    return suite.get("prefix", "BENCHMARKS_DIR")


def normalize_stdout(stdout: str | bytes | None) -> str:
    if stdout is None:
        return ""
    if isinstance(stdout, bytes):
        return stdout.decode(errors="replace")
    return stdout


def load_cases(suite: dict[str, Any]) -> list[dict[str, Any]]:
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
    benchmark_executable: pathlib.Path,
    run_name: str,
    min_time: str,
    result_file: pathlib.Path | None,
    repetitions: int | None,
    report_aggregates_only: bool,
    suite_json: pathlib.Path,
    benchmark_args: list[str],
) -> list[str]:
    # Google Benchmark uses POSIX regex on Unix; Python's re.escape also escapes
    # ordinary characters such as '-' that POSIX rejects outside character sets.
    escaped_name = "".join("\\" + char if char in r"\.^$|?*+()[]{}" else char for char in run_name)
    command = [
        str(benchmark_executable),
        f"--suite-json={suite_json.resolve()}",
        *benchmark_args,
        f"--benchmark_filter=^{escaped_name}/.*",
        f"--benchmark_min_time={min_time}",
        "--benchmark_format=json",
    ]
    if result_file is not None:
        command.extend(
            [
                f"--benchmark_out={result_file}",
                "--benchmark_out_format=json",
            ]
        )
    if repetitions is not None:
        command.append(f"--benchmark_repetitions={repetitions}")
    if report_aggregates_only:
        command.append("--benchmark_report_aggregates_only=true")
    return command


def run_command(command: list[str], timeout: float) -> CommandResult:
    started = time.monotonic()
    try:
        result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
        duration = time.monotonic() - started
        stdout = normalize_stdout(result.stdout)
        exit_code = result.returncode
        status = "passed" if result.returncode == 0 else "failed"
    except subprocess.TimeoutExpired as error:
        duration = time.monotonic() - started
        stdout = normalize_stdout(error.stdout)
        exit_code = None
        status = "timed_out"

    return {
        "command": command,
        "status": status,
        "exit_code": exit_code,
        "duration_seconds": duration,
        "stdout": stdout,
    }


def write_log_entry(log_file: TextIO, run_name: str, phase: str, result: CommandResult) -> None:
    log_file.write(f"===== {run_name} ({phase}: {result['status']}) =====\n")
    log_file.write(result["stdout"])
    if result["stdout"] and not result["stdout"].endswith("\n"):
        log_file.write("\n")


def validate_benchmark_result(path: pathlib.Path, run_name: str) -> str | None:
    try:
        data = require_mapping(json.loads(path.read_text()), "benchmark output")
        raw_benchmarks: object = data["benchmarks"]
        if not isinstance(raw_benchmarks, list) or not raw_benchmarks:
            return "Missing or mismatched benchmark results."
        benchmarks = [require_mapping(item, "benchmark") for item in cast(list[object], raw_benchmarks)]
        if not all(isinstance(item.get("name"), str) and item["name"].startswith(f"{run_name}/") for item in benchmarks):
            return "Missing or mismatched benchmark results."
        errors = [item.get("error_message", "Benchmark error.") for item in benchmarks if item.get("error_occurred")]
        if errors:
            return "; ".join(errors)
        if any(not isinstance(item.get(key), (int, float)) for item in benchmarks for key in ("real_time", "cpu_time")):
            return "Missing or invalid benchmark timings."
        if any(item.get("time_unit") not in {"ns", "us", "ms", "s"} for item in benchmarks):
            return "Missing or invalid benchmark time unit."
        return None
    except (OSError, ValueError, KeyError, TypeError):
        return "Missing or invalid benchmark JSON."


def run_benchmarks(
    benchmark_executable: pathlib.Path,
    output_dir: pathlib.Path,
    cases: list[dict[str, Any]],
    min_time: str,
    repetitions: int | None,
    report_aggregates_only: bool,
    timeout: float,
    suite_json: pathlib.Path,
    benchmark_args: list[str],
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    results: list[dict[str, Any]] = []
    failures: list[dict[str, Any]] = []
    benchmark_output_dir = output_dir / "benchmark-results"
    benchmark_log_file = output_dir / "benchmark.log"

    with benchmark_log_file.open("w") as benchmark_log:
        for case in cases:
            run_name = case["run_name"]
            result_file = benchmark_output_dir / f"{run_name}.json"
            temp_result_file = result_file.with_name(f"{result_file.name}.tmp")
            result_file.parent.mkdir(parents=True, exist_ok=True)
            result_file.unlink(missing_ok=True)
            temp_result_file.unlink(missing_ok=True)

            command = build_benchmark_command(
                benchmark_executable,
                run_name,
                min_time,
                temp_result_file,
                repetitions,
                report_aggregates_only,
                suite_json,
                benchmark_args,
            )
            result = run_command(command, timeout)
            if result["status"] == "passed":
                error = validate_benchmark_result(temp_result_file, run_name)
                if error:
                    result["status"] = "failed"
                    result["stdout"] += f"\n{error}\n"
            write_log_entry(benchmark_log, run_name, "benchmark", result)

            if result["status"] == "passed":
                temp_result_file.replace(result_file)
                results.append(
                    {
                        "run_name": run_name,
                        "command": command,
                        "status": result["status"],
                        "exit_code": result["exit_code"],
                        "result_file": str(result_file),
                        "duration_seconds": result["duration_seconds"],
                    }
                )
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
                failures.append(failure)
                case["benchmark_failure_reason"] = reason
                if result["status"] == "timed_out":
                    case["benchmark_timeout_seconds"] = timeout

            case["status"] = result["status"]
            case["benchmark_status"] = result["status"]
            case["benchmark_duration_seconds"] = result["duration_seconds"]
            case["benchmark_exit_code"] = result["exit_code"]

    return results, failures


def main() -> int:
    parser = argparse.ArgumentParser(description="Run each profiling benchmark case with a hard wall-clock timeout.")
    parser.add_argument("--executable", type=pathlib.Path, required=True, help="Profiling benchmark executable to run.")
    parser.add_argument(
        "--output-dir",
        required=True,
        help="Directory for benchmark.log, summary.json, and benchmark result JSON files.",
    )
    parser.add_argument(
        "--suite-json",
        type=pathlib.Path,
        required=True,
        help="Profiling suite JSON with domains, domain files, and task files.",
    )
    parser.add_argument("--benchmark-min-time", default="0.1s", help="Google Benchmark --benchmark_min_time value.")
    parser.add_argument("--benchmark-repetitions", type=int, help="Google Benchmark --benchmark_repetitions value.")
    parser.add_argument("--benchmark-arg", action="append", default=[], help="Extra executable option; repeat as --benchmark-arg=--cores=1,16.")
    parser.add_argument("--case-filter", help="Regular expression selecting domain/task names from the suite.")
    parser.add_argument(
        "--benchmark-timeout",
        type=float,
        default=60.0,
        help="Hard wall-clock timeout in seconds for each per-case Google Benchmark subprocess.",
    )
    parser.add_argument(
        "--benchmark-report-aggregates-only",
        action="store_true",
        help="Forward --benchmark_report_aggregates_only=true to Google Benchmark.",
    )
    args = parser.parse_args()
    if args.benchmark_timeout <= 0:
        parser.error("--benchmark-timeout must be greater than 0.")

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

    benchmark_results, benchmark_failures = run_benchmarks(
        executable,
        output_dir,
        cases,
        args.benchmark_min_time,
        args.benchmark_repetitions,
        args.benchmark_report_aggregates_only,
        args.benchmark_timeout,
        args.suite_json,
        args.benchmark_arg,
    )
    summary = build_summary(suite, cases, metadata, benchmark_results, benchmark_failures)

    rendered_summary = json.dumps(summary, indent=2)
    summary_file.write_text(rendered_summary + "\n")
    print_summary(summary)

    return summary["exit_code"]


if __name__ == "__main__":
    sys.exit(main())
