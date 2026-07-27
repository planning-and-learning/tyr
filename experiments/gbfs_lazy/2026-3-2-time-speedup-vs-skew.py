#! /usr/bin/env python

import json

from collections import defaultdict
from pathlib import Path

HTG_PREFIX = "classical-htg-domains-"

def load_htg_runs(path):
    with path.open() as file:
        runs = json.load(file).values()
    return {(run["domain"], run["problem"]): run for run in runs if run["domain"].startswith(HTG_PREFIX)}


def main():
    results = Path(__file__).resolve().parent / "results_raw"
    runs_1 = load_htg_runs(results / "2026-1-8-gbfs_lazy-delta-kckp-1-eval/properties")
    runs_8 = load_htg_runs(results / "2026-1-8-gbfs_lazy-delta-kckp-8-eval/properties")
    per_domain_items = defaultdict(set)

    for task in runs_1.keys() & runs_8.keys():
        run_1 = runs_1[task]
        run_8 = runs_8[task]
        if not run_1["coverage"] or not run_8["coverage"] or run_1["total_time_s"] < 6:
            continue

        total_time_ms = run_1["total_time_ns"] / 1_000_000
        datalog_fraction = (run_1["succgen_prog_t_tot_ms"] + run_1["ff_prog_t_tot_ms"]) / total_time_ms
        if datalog_fraction < 0.5:
            continue

        speedup = run_1["total_time_ns"] / run_8["total_time_ns"]
        per_domain_items[task[0]].add((run_8["ff_rule_skew_tot"], speedup))

    for domain, items in per_domain_items.items():
        print()
        print(domain)
        print(len(items))
        for skew, speedup in items:
            print(f"({skew}, {speedup})", end=" ")
        print()



if __name__ == "__main__":
    main()
