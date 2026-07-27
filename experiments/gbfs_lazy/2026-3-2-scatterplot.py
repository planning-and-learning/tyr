#! /usr/bin/env python

import json
from pathlib import Path

from downward.reports.scatter import ScatterPlotReport
from lab.experiment import Experiment
from lab.reports import Attribute


DIR = Path(__file__).resolve().parent
RESULTS_RAW = DIR / "results_raw"
TYR_DIR = RESULTS_RAW / "2026-1-8-gbfs_lazy-delta-kckp-1-eval"
PL_DIR = RESULTS_RAW / "pl-2026-1-9-lazy-gbfs-ff-pref-ff-eval"
TYR_ALGO = "gbfs-lazy-delta-kckp-hff-pref-ff-1"
PL_ALGO = "powerlifted-gbfs-lazy-hff-pref-ff"
ATTRIBUTE = Attribute("search_time_ms_per_expanded", digits=3)


def task_key(run):
    return run["domain"], run["problem"]


def is_htg(run):
    return run["domain"].startswith("classical-htg-domains-") and not run["problem"].endswith("-positional.pddl")


def datalog_fraction(run):
    total_ms = run["total_time_ns"] / 1_000_000
    return sum(run.get(name, 0) for name in ("axiom_prog_t_tot_ms", "succgen_prog_t_tot_ms", "ff_prog_t_tot_ms")) / total_ms


def load_runs(path):
    with (path / "properties").open() as file:
        runs = json.load(file).values()
    return {task_key(run): run for run in runs if is_htg(run)}


def build_buckets():
    tyr_runs = load_runs(TYR_DIR)
    pl_runs = load_runs(PL_DIR)
    buckets = set(), set()

    for task in tyr_runs.keys() & pl_runs.keys():
        tyr_run = tyr_runs[task]
        if tyr_run["coverage"] and pl_runs[task]["coverage"]:
            buckets[datalog_fraction(tyr_run) >= 0.5].add(task)

    return buckets


def add_scatter_experiment(name, allowed=None):
    exp = Experiment(str(DIR / name))

    def task_filter(run):
        return is_htg(run) and (allowed is None or task_key(run) in allowed)

    exp.add_fetcher(str(TYR_DIR), name="fetch-tyr", merge=False, filter=task_filter)
    exp.add_fetcher(str(PL_DIR), name="fetch-powerlifted", merge=True, filter=task_filter)

    for output_format in ("png", "tex"):
        exp.add_report(
            ScatterPlotReport(
                attributes=[ATTRIBUTE],
                filter_algorithm=[PL_ALGO, TYR_ALGO],
                format=output_format,
            ),
            name=f"scatterplot-{output_format}",
        )

    exp.run_steps()


add_scatter_experiment("plot-search-time-ms-per-expanded")

allowed_lt_05, allowed_ge_05 = build_buckets()
add_scatter_experiment("plot-search-time-ms-per-expanded-df-lt-0-5", allowed_lt_05)
add_scatter_experiment("plot-search-time-ms-per-expanded-df-ge-0-5", allowed_ge_05)
