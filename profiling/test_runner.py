# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Tests of the profiling runner, report, and compare scripts.

This module lives outside the repository's test folders, so it checks with ``check`` instead of ``assert``.
"""

import json
import math
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).parent))

import compare
import report
import runner


def check(actual: object, expected: object, context: object = "") -> None:
    """Fail the calling test unless ``actual`` equals ``expected``.

    Raises:
        AssertionError: If they differ; the message shows both and the optional context.
    """
    if actual != expected:
        msg = f"{actual!r} != {expected!r} {context}".rstrip()
        raise AssertionError(msg)


def close(actual: object, expected: float) -> bool:
    """Whether ``actual`` is a number within 1e-7 of ``expected``, the tolerance of unittest's assertAlmostEqual."""
    return isinstance(actual, (int, float)) and math.isclose(actual, expected, rel_tol=0, abs_tol=1e-7)


def benchmark(name: str | int, real_time: float, **fields: object) -> dict[str, Any]:
    """A Google Benchmark record in nanoseconds whose CPU time is half its real time."""
    return {"name": name, "real_time": real_time, "cpu_time": real_time / 2, "time_unit": "ns", **fields}


class ProfilingRunnerTest(unittest.TestCase):
    """Runner command line, result validation, representative selection, and comparison."""

    def test_command_forwards_suite_and_options_and_escapes_filter(self) -> None:
        """The command forwards suite and options, and its filter matches exactly the run name's benchmarks."""
        with tempfile.TemporaryDirectory() as directory:
            suite = Path(directory) / "suite with spaces.json"
            run_name = "domain-name +[v2]/p(01).pddl"
            options = runner.BenchmarkOptions(
                suite_json=suite,
                benchmark_args=["--cores=1,16", "--max-states=1000"],
                min_time="0.1s",
                repetitions=3,
                report_aggregates_only=True,
                timeout=60.0,
            )
            command = runner.build_benchmark_command(Path("astar_blind_ground"), run_name, None, options)
            expected_arguments = [
                f"--suite-json={suite.resolve()}",
                "--cores=1,16",
                "--max-states=1000",
                "--benchmark_repetitions=3",
                "--benchmark_report_aggregates_only=true",
            ]
            check([argument for argument in expected_arguments if argument not in command], [])
            pattern = next(
                argument.split("=", 1)[1] for argument in command if argument.startswith("--benchmark_filter=")
            )
            check([escape for escape in (r"\-", r"\ ") if escape in pattern], [])
            check(re.fullmatch(pattern, run_name + "/astar_blind/workers:16") is not None, expected=True)
            check(re.fullmatch(pattern, "domainvv2/p001Xpddl/astar_blind/workers:16"), None)
            check(re.fullmatch(pattern, "prefix/" + run_name + "/astar_blind/workers:16"), None)

    def test_results_require_valid_nonempty_matching_error_free_json(self) -> None:
        """Only an existing, non-empty, matching, error-free benchmark JSON validates."""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "results.json"
            check(runner.validate_benchmark_result(path, "domain/task") is not None, expected=True)
            invalid = [
                "not json",
                "{}",
                json.dumps({"benchmarks": []}),
                json.dumps({"benchmarks": [benchmark("other/task/search", 1)]}),
                json.dumps(
                    {"benchmarks": [benchmark("domain/task/search", 1, error_occurred=True, error_message="failed")]}
                ),
                json.dumps({"benchmarks": [benchmark(42, 1)]}),
                json.dumps({"benchmarks": [{"name": "domain/task/search"}]}),
                json.dumps({"benchmarks": [benchmark("domain/task/search", 1, cpu_time="bad")]}),
                json.dumps({"benchmarks": [benchmark("domain/task/search", 1, time_unit="ticks")]}),
            ]
            for document in invalid:
                with self.subTest(document=document):
                    path.write_text(document)
                    check(runner.validate_benchmark_result(path, "domain/task") is not None, expected=True)
            path.write_text(json.dumps({"benchmarks": [benchmark("domain/task/search", 1)]}))
            check(runner.validate_benchmark_result(path, "domain/task"), None)

    def test_representatives_keep_every_worker_variant_and_prefer_medians(self) -> None:
        """Every worker variant keeps its own representative, the reported median."""
        names = [f"domain/task/astar_blind/sync/workers:{workers}" for workers in (1, 16)]
        records: list[dict[str, Any]] = []
        for name, median in zip(names, (10, 4), strict=True):
            records.extend(
                [
                    benchmark(name, 99, run_name=name, run_type="iteration"),
                    benchmark(name + "_mean", 50, run_name=name, run_type="aggregate", aggregate_name="mean"),
                    benchmark(name + "_median", median, run_name=name, run_type="aggregate", aggregate_name="median"),
                ]
            )
        records.append(benchmark("domain/task/broken", 0, error_occurred=True))
        selected = report.representative_benchmarks(records)
        check(set(selected), set(names))
        check([selected[name]["real_time"] for name in names], [10, 4])

    def test_representatives_fall_back_to_median_repetitions(self) -> None:
        """Without a median aggregate, every number is the median over the repetitions; inputs stay unchanged."""
        name = "domain/task/astar_blind/sync/workers:16"
        records = [benchmark(name, value, run_name=name, run_type="iteration", workers=16) for value in (9, 1, 5)]
        records.append(benchmark(name + "_mean", 100, run_name=name, run_type="aggregate", aggregate_name="mean"))
        selected = report.representative_benchmarks(records)[name]
        check((selected["real_time"], selected["cpu_time"], selected["workers"]), (5, 2.5, 16))
        check(records[0]["real_time"], 9)

    def test_representatives_preserve_mixed_solved_fraction(self) -> None:
        """A fractional solved mean is kept and marks the representative mixed; inputs stay unchanged."""
        name = "domain/task/astar_blind/sync/workers:16"
        iterations = [
            benchmark(name, value, solved=solved, label="SOLVED" if solved else "OUT_OF_TIME")
            for value, solved in ((1, 1), (3, 1), (10, 0))
        ]
        aggregates = [
            benchmark(name + "_mean", 14 / 3, aggregate_name="mean", solved=2 / 3),
            benchmark(name + "_median", 3, aggregate_name="median", solved=1, label="SOLVED"),
        ]
        for records in (iterations, aggregates):
            with self.subTest(records=records):
                selected = report.representative_benchmarks(records)[name]
                check(selected["real_time"], 3)
                check(close(selected["solved"], 2 / 3), expected=True, context=selected["solved"])
                check(selected["label"], "mixed")
        check((aggregates[1]["solved"], aggregates[1]["label"]), (1, "SOLVED"))

    def test_compare_reads_old_aggregate_only_names(self) -> None:
        """Aggregate-only records without run_name are matched by their name minus the aggregate suffix."""
        name = "domain/task/astar_blind/sync/workers:1"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "old-results.json").write_text(
                json.dumps(
                    {
                        "benchmarks": [
                            benchmark(name + "_mean", 20, run_type="aggregate", aggregate_name="mean"),
                            benchmark(name + "_median", 10, run_type="aggregate", aggregate_name="median"),
                        ]
                    }
                )
            )
            summary = {"benchmark_results": [{"exit_code": 0, "result_file": "old-results.json"}]}
            old = compare.load_benchmarks(root / "summary.json", summary)
            check(set(old), {name})
            (comparison,) = compare.compare_benchmarks(old, {name: benchmark(name, 5)})
            check(comparison["real_time_ratio"], 0.5)

    def test_compare_converts_old_timings_to_new_unit(self) -> None:
        """Old timings are converted to the new run's time unit before ratios and deltas; inputs stay unchanged."""
        name = "domain/task/search"
        for old_unit, old_time, new_unit, new_time in (
            ("ns", 2000, "us", 1),
            ("s", 2, "ms", 1000),
            ("us", 2, "ns", 1000),
        ):
            with self.subTest(old_unit=old_unit, new_unit=new_unit):
                old = benchmark(name, old_time, time_unit=old_unit)
                new = benchmark(name, new_time, time_unit=new_unit)
                (row,) = compare.compare_benchmarks({name: old}, {name: new})
                check(row["time_unit"], new_unit)
                expected_values = {
                    "old_real_time": new_time * 2,
                    "old_cpu_time": new_time,
                    "real_time_ratio": 0.5,
                    "cpu_time_ratio": 0.5,
                    "real_time_delta_percent": -50,
                    "cpu_time_delta_percent": -50,
                }
                check({key: row[key] for key, value in expected_values.items() if not close(row[key], value)}, {})
                check(old["real_time"], old_time)

    def test_compare_rejects_missing_variants_and_empty_comparisons(self) -> None:
        """compare.py fails unless both runs have the same, non-empty set of benchmarks, and counts the differences."""
        one = "domain/task/search/workers:1"
        sixteen = "domain/task/search/workers:16"
        for old_names, new_names in (
            ([one], [one]),
            ([one, sixteen], [one]),
            ([one], [one, sixteen]),
            ([one], [sixteen]),
            ([], []),
        ):
            with self.subTest(old=old_names, new=new_names), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                for side, names in (("old", old_names), ("new", new_names)):
                    results = root / f"{side}-results.json"
                    results.write_text(json.dumps({"benchmarks": [benchmark(name, 10) for name in names]}))
                    (root / f"{side}.json").write_text(
                        json.dumps(
                            {
                                "cases": [
                                    {"run_name": "domain/task", "status": "passed", "benchmark_status": "passed"}
                                ],
                                "benchmark_results": [{"exit_code": 0, "result_file": str(results)}],
                            }
                        )
                    )
                result = subprocess.run(
                    [sys.executable, str(Path(compare.__file__)), str(root / "old.json"), str(root / "new.json")],
                    text=True,
                    capture_output=True,
                    check=False,
                )
                check(result.returncode, 0 if old_names == new_names and old_names else 1, context=result.stderr)
                comparison = json.loads(result.stdout)
                check(comparison["missing_in_old"], sorted(set(new_names) - set(old_names)))
                check(comparison["missing_in_new"], sorted(set(old_names) - set(new_names)))
                check(
                    (
                        comparison["counts"]["missing_in_old"],
                        comparison["counts"]["missing_in_new"],
                        comparison["counts"]["matched_benchmarks"],
                    ),
                    (
                        len(comparison["missing_in_old"]),
                        len(comparison["missing_in_new"]),
                        len(set(old_names) & set(new_names)),
                    ),
                )


if __name__ == "__main__":
    unittest.main()
