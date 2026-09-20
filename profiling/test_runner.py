import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from typing import Any, cast
import unittest

sys.path.insert(0, str(Path(__file__).parent))

import compare
import report
import runner


def benchmark(name: str | int, real_time: float, **fields: Any) -> dict[str, Any]:
    return {"name": name, "real_time": real_time, "cpu_time": real_time / 2, "time_unit": "ns", **fields}


class ProfilingRunnerTest(unittest.TestCase):
    def test_command_forwards_suite_and_options_and_escapes_filter(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            suite = Path(directory) / "suite with spaces.json"
            run_name = "domain-name +[v2]/p(01).pddl"
            command = runner.build_benchmark_command(
                Path("astar_blind_ground"), run_name, "0.1s", None, 3, True, suite, ["--cores=1,16", "--max-states=1000"]
            )
            self.assertIn(f"--suite-json={suite.resolve()}", command)
            self.assertIn("--cores=1,16", command)
            self.assertIn("--max-states=1000", command)
            self.assertIn("--benchmark_repetitions=3", command)
            self.assertIn("--benchmark_report_aggregates_only=true", command)
            pattern = next(argument.split("=", 1)[1] for argument in command if argument.startswith("--benchmark_filter="))
            self.assertNotIn(r"\-", pattern)
            self.assertNotIn(r"\ ", pattern)
            self.assertIsNotNone(re.fullmatch(pattern, run_name + "/astar_blind/workers:16"))
            self.assertIsNone(re.fullmatch(pattern, "domainvv2/p001Xpddl/astar_blind/workers:16"))
            self.assertIsNone(re.fullmatch(pattern, "prefix/" + run_name + "/astar_blind/workers:16"))

    def test_results_require_valid_nonempty_matching_error_free_json(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "results.json"
            self.assertIsNotNone(runner.validate_benchmark_result(path, "domain/task"))
            invalid = [
                "not json",
                "{}",
                json.dumps({"benchmarks": []}),
                json.dumps({"benchmarks": [benchmark("other/task/search", 1)]}),
                json.dumps({"benchmarks": [benchmark("domain/task/search", 1, error_occurred=True, error_message="failed")]}),
                json.dumps({"benchmarks": [benchmark(42, 1)]}),
                json.dumps({"benchmarks": [{"name": "domain/task/search"}]}),
                json.dumps({"benchmarks": [benchmark("domain/task/search", 1, cpu_time="bad")]}),
                json.dumps({"benchmarks": [benchmark("domain/task/search", 1, time_unit="ticks")]}),
            ]
            for document in invalid:
                with self.subTest(document=document):
                    path.write_text(document)
                    self.assertIsNotNone(runner.validate_benchmark_result(path, "domain/task"))
            path.write_text(json.dumps({"benchmarks": [benchmark("domain/task/search", 1)]}))
            self.assertIsNone(runner.validate_benchmark_result(path, "domain/task"))

    def test_representatives_keep_every_worker_variant_and_prefer_medians(self) -> None:
        names = [f"domain/task/astar_blind/sync/workers:{workers}" for workers in (1, 16)]
        records: list[dict[str, Any]] = []
        for name, median in zip(names, (10, 4)):
            records.extend([
                benchmark(name, 99, run_name=name, run_type="iteration"),
                benchmark(name + "_mean", 50, run_name=name, run_type="aggregate", aggregate_name="mean"),
                benchmark(name + "_median", median, run_name=name, run_type="aggregate", aggregate_name="median"),
            ])
        records.append(benchmark("domain/task/broken", 0, error_occurred=True))
        selected = report.representative_benchmarks(records)
        self.assertEqual(set(selected), set(names))
        self.assertEqual([selected[name]["real_time"] for name in names], [10, 4])

    def test_representatives_fall_back_to_median_repetitions(self) -> None:
        name = "domain/task/astar_blind/sync/workers:16"
        records = [benchmark(name, value, run_name=name, run_type="iteration", workers=16) for value in (9, 1, 5)]
        records.append(benchmark(name + "_mean", 100, run_name=name, run_type="aggregate", aggregate_name="mean"))
        selected = report.representative_benchmarks(records)[name]
        self.assertEqual(selected["real_time"], 5)
        self.assertEqual(selected["cpu_time"], 2.5)
        self.assertEqual(selected["workers"], 16)
        self.assertEqual(records[0]["real_time"], 9)

    def test_representatives_preserve_mixed_solved_fraction(self) -> None:
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
                self.assertEqual(selected["real_time"], 3)
                self.assertAlmostEqual(selected["solved"], 2 / 3)
                self.assertEqual(selected["label"], "mixed")
        self.assertEqual(aggregates[1]["solved"], 1)
        self.assertEqual(aggregates[1]["label"], "SOLVED")

    def test_compare_reads_old_aggregate_only_names(self) -> None:
        name = "domain/task/astar_blind/sync/workers:1"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "old-results.json").write_text(json.dumps({"benchmarks": [
                benchmark(name + "_mean", 20, run_type="aggregate", aggregate_name="mean"),
                benchmark(name + "_median", 10, run_type="aggregate", aggregate_name="median"),
            ]}))
            summary = {"benchmark_results": [{"exit_code": 0, "result_file": "old-results.json"}]}
            old = compare.load_benchmarks(root / "summary.json", summary)
            self.assertEqual(set(old), {name})
            comparison, = compare.compare_benchmarks(old, {name: benchmark(name, 5)})
            self.assertEqual(comparison["real_time_ratio"], 0.5)

    def test_compare_converts_old_timings_to_new_unit(self) -> None:
        name = "domain/task/search"
        for old_unit, old_time, new_unit, new_time in (("ns", 2000, "us", 1), ("s", 2, "ms", 1000), ("us", 2, "ns", 1000)):
            with self.subTest(old_unit=old_unit, new_unit=new_unit):
                old = benchmark(name, old_time, time_unit=old_unit)
                new = benchmark(name, new_time, time_unit=new_unit)
                row, = compare.compare_benchmarks({name: old}, {name: new})
                self.assertEqual(row["time_unit"], new_unit)
                self.assertAlmostEqual(cast(float, row["old_real_time"]), new_time * 2)
                self.assertAlmostEqual(cast(float, row["old_cpu_time"]), new_time)
                self.assertAlmostEqual(cast(float, row["real_time_ratio"]), 0.5)
                self.assertAlmostEqual(cast(float, row["cpu_time_ratio"]), 0.5)
                self.assertAlmostEqual(cast(float, row["real_time_delta_percent"]), -50)
                self.assertAlmostEqual(cast(float, row["cpu_time_delta_percent"]), -50)
                self.assertEqual(old["real_time"], old_time)

    def test_compare_rejects_missing_variants_and_empty_comparisons(self) -> None:
        one = "domain/task/search/workers:1"
        sixteen = "domain/task/search/workers:16"
        for old_names, new_names in (([one], [one]), ([one, sixteen], [one]), ([one], [one, sixteen]), ([one], [sixteen]), ([], [])):
            with self.subTest(old=old_names, new=new_names), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                for side, names in (("old", old_names), ("new", new_names)):
                    results = root / f"{side}-results.json"
                    results.write_text(json.dumps({"benchmarks": [benchmark(name, 10) for name in names]}))
                    (root / f"{side}.json").write_text(json.dumps({
                        "cases": [{"run_name": "domain/task", "status": "passed", "benchmark_status": "passed"}],
                        "benchmark_results": [{"exit_code": 0, "result_file": str(results)}],
                    }))
                result = subprocess.run(
                    [sys.executable, str(Path(compare.__file__)), str(root / "old.json"), str(root / "new.json")],
                    text=True, capture_output=True,
                )
                self.assertEqual(result.returncode, 0 if old_names == new_names and old_names else 1, result.stderr)
                comparison = json.loads(result.stdout)
                self.assertEqual(comparison["missing_in_old"], sorted(set(new_names) - set(old_names)))
                self.assertEqual(comparison["missing_in_new"], sorted(set(old_names) - set(new_names)))
                self.assertEqual(comparison["counts"]["missing_in_old"], len(comparison["missing_in_old"]))
                self.assertEqual(comparison["counts"]["missing_in_new"], len(comparison["missing_in_new"]))
                self.assertEqual(comparison["counts"]["matched_benchmarks"], len(set(old_names) & set(new_names)))


if __name__ == "__main__":
    unittest.main()
