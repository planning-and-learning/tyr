import datetime as dt
import json
import pathlib
import statistics
import sys
from typing import Any, TextIO

from schema import normalize_attribute_value


def load_json(path: pathlib.Path) -> dict[str, Any]:
    return json.loads(path.read_text())


def representative_benchmarks(benchmarks: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    """Prefer Google Benchmark's median; otherwise take the median of repetitions."""
    groups: dict[str, list[dict[str, Any]]] = {}
    for benchmark in benchmarks:
        if benchmark.get("error_occurred"):
            continue
        name = benchmark.get("run_name", benchmark["name"])
        if "run_name" not in benchmark and benchmark.get("aggregate_name"):
            name = name.removesuffix("_" + benchmark["aggregate_name"])
        groups.setdefault(name, []).append(benchmark)

    result: dict[str, dict[str, Any]] = {}
    for name, records in groups.items():
        median = next((record for record in records if record.get("aggregate_name") == "median"), None)
        iterations = [record for record in records if record.get("run_type", "iteration") == "iteration"]
        if median is not None:
            result[name] = dict(median)
        elif iterations:
            row = dict(iterations[0])
            for key, value in row.items():
                if isinstance(value, (int, float)) and not isinstance(value, bool):
                    row[key] = statistics.median(record[key] for record in iterations)
            if len({record.get("label") for record in iterations}) > 1:
                row["label"] = "mixed"
            result[name] = row
        if name in result and "solved" in result[name]:
            mean = next((record for record in records if record.get("aggregate_name") == "mean"), None)
            if mean is not None:
                result[name]["solved"] = mean["solved"]
            elif iterations:
                result[name]["solved"] = statistics.mean(record["solved"] for record in iterations)
            if 0 < result[name]["solved"] < 1:
                result[name]["label"] = "mixed"
    return result


def build_summary(
    suite: dict[str, Any],
    cases: list[dict[str, Any]],
    metadata: dict[str, Any],
    benchmark_results: list[dict[str, Any]],
    benchmark_failures: list[dict[str, Any]],
) -> dict[str, Any]:
    groups = {status: [case for case in cases if case["status"] == status] for status in ("passed", "timed_out", "failed", "not_run")}
    attributes: dict[str, dict[str, Any]] = suite.get("attributes", {})
    rows: list[dict[str, Any]] = []
    for case in groups["passed"]:
        data = load_json(pathlib.Path(case["benchmark_result_file"]))
        for name, benchmark in representative_benchmarks(data["benchmarks"]).items():
            rows.append({
                "name": name,
                "real_time": benchmark["real_time"],
                "cpu_time": benchmark["cpu_time"],
                "time_unit": benchmark["time_unit"],
                "label": benchmark.get("label", ""),
                "attributes": {key: normalize_attribute_value(key, config, benchmark.get(key)) for key, config in attributes.items()},
            })

    return {
        "generated": dt.datetime.now().astimezone().isoformat(timespec="seconds"),
        "metadata": metadata,
        "attributes": attributes,
        "exit_code": 1 if groups["failed"] or groups["timed_out"] else 0,
        "cases": cases,
        **groups,
        "benchmark_results": benchmark_results,
        "benchmark_failures": benchmark_failures,
        "benchmark_summaries": rows,
        "counts": {**{status: len(values) for status, values in groups.items()}, "total": len(cases)},
    }


def format_summary_value(value: object) -> str:
    if value is None:
        return "-"
    if isinstance(value, float):
        return f"{value:.6g}"
    return str(value)


def print_summary(summary: dict[str, Any], file: TextIO | None = None) -> None:
    if file is None:
        file = sys.stdout
    attribute_names = [name for name in summary.get("attributes", {})
                       if any(row["attributes"].get(name) is not None for row in summary.get("benchmark_summaries", []))]
    columns = ["benchmark", "wall time", "status", *attribute_names]
    rows = [[
        entry["name"],
        f"{entry['real_time']:.6g} {entry['time_unit']}",
        entry["label"],
        *[format_summary_value(entry["attributes"].get(name)) for name in attribute_names],
    ] for entry in summary.get("benchmark_summaries", [])]
    rows += [[case["run_name"], "-", case["status"], *["-" for _ in attribute_names]]
             for case in summary.get("cases", []) if case["status"] != "passed"]
    widths = [max(len(column), *(len(row[i]) for row in rows)) if rows else len(column) for i, column in enumerate(columns)]
    for row in [columns, ["-" * width for width in widths], *rows]:
        print("  ".join(cell.ljust(width) for cell, width in zip(row, widths)), file=file)
    print("Cases: " + ", ".join(f"{count} {status}" for status, count in summary["counts"].items()), file=file)
