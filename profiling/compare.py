#!/usr/bin/env python3
import argparse
import json
import operator
import pathlib
import sys
from typing import Any

from schema import AttributeCompare, AttributeValue, normalize_attribute_value, validate_attributes
from report import representative_benchmarks


def load_json(path: pathlib.Path) -> dict[str, Any]:
    return json.loads(path.read_text())


def resolve_result_file(summary_path: pathlib.Path, result_file: str) -> pathlib.Path:
    path = pathlib.Path(result_file)
    if path.exists():
        return path

    relative_path = summary_path.parent / path
    if relative_path.exists():
        return relative_path

    return path


def load_benchmarks(summary_path: pathlib.Path, summary: dict[str, Any]) -> dict[str, dict[str, Any]]:
    benchmarks: dict[str, dict[str, Any]] = {}

    for result in summary.get("benchmark_results", []):
        if result.get("exit_code") != 0:
            continue

        result_file = result.get("result_file")
        if not result_file:
            continue

        path = resolve_result_file(summary_path, result_file)
        if not path.exists():
            continue

        data = load_json(path)
        benchmarks.update(representative_benchmarks(data.get("benchmarks", [])))

    return benchmarks


def index_cases(summary: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {case["run_name"]: case for case in summary.get("cases", [])}


def compare_cases(old_summary: dict[str, Any], new_summary: dict[str, Any]) -> list[dict[str, str | None]]:
    old_cases = index_cases(old_summary)
    new_cases = index_cases(new_summary)
    run_names = sorted(set(old_cases) | set(new_cases))

    changes: list[dict[str, str | None]] = []
    for run_name in run_names:
        old_case = old_cases.get(run_name)
        new_case = new_cases.get(run_name)

        old_status = old_case.get("status") if old_case else "missing"
        new_status = new_case.get("status") if new_case else "missing"
        old_benchmark_status = old_case.get("benchmark_status") if old_case else "missing"
        new_benchmark_status = new_case.get("benchmark_status") if new_case else "missing"

        if old_status != new_status or old_benchmark_status != new_benchmark_status:
            changes.append(
                {
                    "run_name": run_name,
                    "old_status": old_status,
                    "new_status": new_status,
                    "old_benchmark_status": old_benchmark_status,
                    "new_benchmark_status": new_benchmark_status,
                }
            )

    return changes


def compare_benchmarks(
    old_benchmarks: dict[str, dict[str, Any]], new_benchmarks: dict[str, dict[str, Any]]
) -> list[dict[str, str | float]]:
    names = sorted(set(old_benchmarks) & set(new_benchmarks))
    comparisons: list[dict[str, str | float]] = []
    seconds_per_unit = {"ns": 1e-9, "us": 1e-6, "ms": 1e-3, "s": 1}

    for name in names:
        old = old_benchmarks[name]
        new = new_benchmarks[name]
        scale = seconds_per_unit[old["time_unit"]] / seconds_per_unit[new["time_unit"]]
        old_real_time = old["real_time"] * scale
        old_cpu_time = old["cpu_time"] * scale
        comparison: dict[str, str | float] = {
            "name": name,
            "old_real_time": old_real_time,
            "new_real_time": new["real_time"],
            "old_cpu_time": old_cpu_time,
            "new_cpu_time": new["cpu_time"],
            "time_unit": new["time_unit"],
        }

        if old_real_time and new["real_time"]:
            comparison["real_time_ratio"] = new["real_time"] / old_real_time
            comparison["real_time_delta_percent"] = 100.0 * (new["real_time"] - old_real_time) / old_real_time

        if old_cpu_time and new["cpu_time"]:
            comparison["cpu_time_ratio"] = new["cpu_time"] / old_cpu_time
            comparison["cpu_time_delta_percent"] = 100.0 * (new["cpu_time"] - old_cpu_time) / old_cpu_time

        comparisons.append(comparison)

    return comparisons


def compare_attribute_value(old_value: AttributeValue, new_value: AttributeValue, rule: str | None) -> str:
    if old_value is None or new_value is None:
        return "missing"

    if rule == AttributeCompare.UNDEFINED.value:
        return "undefined"

    if rule == AttributeCompare.STRICT_EQUALITY.value:
        return "unchanged" if old_value == new_value else "changed"

    if rule == AttributeCompare.LOWER_IS_BETTER.value:
        if operator.lt(new_value, old_value):
            return "improved"
        if operator.gt(new_value, old_value):
            return "regressed"
        return "unchanged"

    if rule == AttributeCompare.HIGHER_IS_BETTER.value:
        if operator.gt(new_value, old_value):
            return "improved"
        if operator.lt(new_value, old_value):
            return "regressed"
        return "unchanged"

    return "unknown_rule"


def compare_attributes(
    old_benchmarks: dict[str, dict[str, Any]],
    new_benchmarks: dict[str, dict[str, Any]],
    attributes: dict[str, dict[str, Any]],
) -> list[dict[str, AttributeValue]]:
    names = sorted(set(old_benchmarks) & set(new_benchmarks))
    comparisons: list[dict[str, AttributeValue]] = []

    for benchmark_name in names:
        old = old_benchmarks[benchmark_name]
        new = new_benchmarks[benchmark_name]

        for attribute_name, config in attributes.items():
            old_value = old.get(attribute_name)
            new_value = new.get(attribute_name)
            if old_value is None and new_value is None:
                continue
            rule = config.get("compare")
            old_value = normalize_attribute_value(attribute_name, config, old_value)
            new_value = normalize_attribute_value(attribute_name, config, new_value)
            status = compare_attribute_value(old_value, new_value, rule)
            comparison: dict[str, AttributeValue] = {
                "benchmark": benchmark_name,
                "attribute": attribute_name,
                "compare": rule,
                "old_value": old_value,
                "new_value": new_value,
                "status": status,
            }

            if isinstance(old_value, (int, float)) and isinstance(new_value, (int, float)) and old_value:
                comparison["ratio"] = new_value / old_value
                comparison["delta_percent"] = 100.0 * (new_value - old_value) / old_value

            comparisons.append(comparison)

    return comparisons


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare two profiling summary JSON files.")
    parser.add_argument("old_summary", type=pathlib.Path)
    parser.add_argument("new_summary", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path, help="Optional JSON file to write the comparison to.")
    args = parser.parse_args()

    old_summary = load_json(args.old_summary)
    new_summary = load_json(args.new_summary)
    old_benchmarks = load_benchmarks(args.old_summary, old_summary)
    new_benchmarks = load_benchmarks(args.new_summary, new_summary)
    attributes: dict[str, dict[str, Any]] = new_summary.get("attributes") or old_summary.get("attributes") or {}
    validate_attributes(attributes)
    attribute_comparisons = compare_attributes(old_benchmarks, new_benchmarks, attributes)

    attribute_violations = [
        comparison
        for comparison in attribute_comparisons
        if comparison["status"] in {"changed", "regressed", "missing", "unknown_rule"}
    ]
    case_status_changes = compare_cases(old_summary, new_summary)
    missing_in_old = sorted(set(new_benchmarks) - set(old_benchmarks))
    missing_in_new = sorted(set(old_benchmarks) - set(new_benchmarks))
    matched = set(old_benchmarks) & set(new_benchmarks)

    result = {
        "old_summary": str(args.old_summary),
        "new_summary": str(args.new_summary),
        "case_status_changes": case_status_changes,
        "attribute_comparisons": attribute_comparisons,
        "attribute_violations": attribute_violations,
        "missing_in_old": missing_in_old,
        "missing_in_new": missing_in_new,
        "benchmark_comparisons": compare_benchmarks(old_benchmarks, new_benchmarks),
        "counts": {
            "old_benchmarks": len(old_benchmarks),
            "new_benchmarks": len(new_benchmarks),
            "matched_benchmarks": len(matched),
            "missing_in_old": len(missing_in_old),
            "missing_in_new": len(missing_in_new),
            "attribute_comparisons": len(attribute_comparisons),
            "attribute_violations": len(attribute_violations),
            "case_status_changes": len(case_status_changes),
        },
    }

    rendered = json.dumps(result, indent=2)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered + "\n")
    else:
        print(rendered)

    if attribute_violations or case_status_changes or missing_in_old or missing_in_new or not matched:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
