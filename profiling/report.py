# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Summarize a profiling run: representative benchmark records, the summary JSON, and its table."""

import datetime as dt
import json
import pathlib
import statistics
import sys
from typing import Any, TextIO

from schema import normalize_attribute_value


def load_json(path: pathlib.Path) -> dict[str, Any]:
    """Read a JSON object from ``path``."""
    return json.loads(path.read_text())


def group_by_run(benchmarks: list[dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    """Error-free records grouped by run name; old aggregate-only output lacks run_name, so strip the suffix."""
    groups: dict[str, list[dict[str, Any]]] = {}
    for benchmark in benchmarks:
        if benchmark.get("error_occurred"):
            continue
        name = benchmark.get("run_name", benchmark["name"])
        if "run_name" not in benchmark and benchmark.get("aggregate_name"):
            name = name.removesuffix("_" + benchmark["aggregate_name"])
        groups.setdefault(name, []).append(benchmark)
    return groups


def median_of_iterations(iterations: list[dict[str, Any]]) -> dict[str, Any]:
    """The first iteration with every number replaced by the median over all iterations; differing labels: mixed."""
    row = dict(iterations[0])
    for key, value in row.items():
        if isinstance(value, (int, float)) and not isinstance(value, bool):
            row[key] = statistics.median(record[key] for record in iterations)
    if len({record.get("label") for record in iterations}) > 1:
        row["label"] = "mixed"
    return row


def representative(records: list[dict[str, Any]]) -> dict[str, Any] | None:
    """One run's representative record; a fractional solved mean marks it mixed. None without median or iterations."""
    median = next((record for record in records if record.get("aggregate_name") == "median"), None)
    iterations = [record for record in records if record.get("run_type", "iteration") == "iteration"]
    if median is not None:
        row = dict(median)
    elif iterations:
        row = median_of_iterations(iterations)
    else:
        return None
    if "solved" in row:
        mean = next((record for record in records if record.get("aggregate_name") == "mean"), None)
        if mean is not None:
            row["solved"] = mean["solved"]
        elif iterations:
            row["solved"] = statistics.mean(record["solved"] for record in iterations)
        if 0 < row["solved"] < 1:
            row["label"] = "mixed"
    return row


def representative_benchmarks(benchmarks: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    """Prefer Google Benchmark's median; otherwise take the median of repetitions."""
    rows = {name: representative(records) for name, records in group_by_run(benchmarks).items()}
    return {name: row for name, row in rows.items() if row is not None}


def build_summary(
    suite: dict[str, Any],
    cases: list[dict[str, Any]],
    metadata: dict[str, Any],
    benchmark_results: list[dict[str, Any]],
    benchmark_failures: list[dict[str, Any]],
) -> dict[str, Any]:
    """The run's summary: cases grouped by status, counts, and each passed benchmark's timing and attributes."""
    groups = {
        status: [case for case in cases if case["status"] == status]
        for status in ("passed", "timed_out", "failed", "not_run")
    }
    attributes: dict[str, dict[str, Any]] = suite.get("attributes", {})
    rows: list[dict[str, Any]] = []
    for case in groups["passed"]:
        data = load_json(pathlib.Path(case["benchmark_result_file"]))
        for name, benchmark in representative_benchmarks(data["benchmarks"]).items():
            rows.append(
                {
                    "name": name,
                    "real_time": benchmark["real_time"],
                    "cpu_time": benchmark["cpu_time"],
                    "time_unit": benchmark["time_unit"],
                    "label": benchmark.get("label", ""),
                    "attributes": {
                        key: normalize_attribute_value(key, config, benchmark.get(key))
                        for key, config in attributes.items()
                    },
                }
            )

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
    """Render an attribute value for the summary table: "-" for None, six significant digits for floats."""
    if value is None:
        return "-"
    if isinstance(value, float):
        return f"{value:.6g}"
    return str(value)


def print_summary(summary: dict[str, Any], file: TextIO | None = None) -> None:
    """Print the summary as a table of benchmarks and unpassed cases, then the case counts, to ``file`` or stdout."""
    if file is None:
        file = sys.stdout
    attribute_names = [
        name
        for name in summary.get("attributes", {})
        if any(row["attributes"].get(name) is not None for row in summary.get("benchmark_summaries", []))
    ]
    columns = ["benchmark", "wall time", "status", *attribute_names]
    rows = [
        [
            entry["name"],
            f"{entry['real_time']:.6g} {entry['time_unit']}",
            entry["label"],
            *[format_summary_value(entry["attributes"].get(name)) for name in attribute_names],
        ]
        for entry in summary.get("benchmark_summaries", [])
    ]
    rows += [
        [case["run_name"], "-", case["status"], *["-" for _ in attribute_names]]
        for case in summary.get("cases", [])
        if case["status"] != "passed"
    ]
    widths = [
        max(len(column), *(len(row[i]) for row in rows)) if rows else len(column) for i, column in enumerate(columns)
    ]
    for row in [columns, ["-" * width for width in widths], *rows]:
        print("  ".join(cell.ljust(width) for cell, width in zip(row, widths, strict=True)), file=file)
    print("Cases: " + ", ".join(f"{count} {status}" for status, count in summary["counts"].items()), file=file)
