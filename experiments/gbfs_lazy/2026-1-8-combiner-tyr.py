#! /usr/bin/env python

import sys 

from pathlib import Path

from lab.experiment import Experiment
from downward.reports.absolute import AbsoluteReport

from lab.reports import Attribute, geometric_mean

DIR = Path(__file__).resolve().parent
REPO = DIR.parent.parent

sys.path.append(str(DIR.parent.parent))

from experiments.parser_datalog import DatalogParser
from experiments.parser_search import SearchParser


# Create custom report class with suitable info and error attributes.
class BaseReport(AbsoluteReport):
    INFO_ATTRIBUTES = ["wall_time_limit", "memory_limit"]
    ERROR_ATTRIBUTES = [
        "domain",
        "problem",
        "algorithm",
        "unexplained_errors",
        "error",
        "node",
    ]

ATTRIBUTES = [
    "run_dir",
]
ATTRIBUTES += SearchParser.get_attributes()
ATTRIBUTES += DatalogParser.get_attributes()



exp = Experiment("tyr-2026-1-8-gbfs_lazy-combined")

def rename_algorithm_delta_kckp(properties):
    """Rename algorithm dynamically during fetching."""
    if properties["algorithm"] == "gbfs-lazy-delta-kckp-hff-pref-ff-1":
        properties["algorithm"] = "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-1"
        properties["id"][0] = "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-1"
    elif properties["algorithm"] == "gbfs-lazy-delta-kckp-hff-pref-ff-2":
        properties["algorithm"] = "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-2"
        properties["id"][0] = "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-2"
    elif properties["algorithm"] == "gbfs-lazy-delta-kckp-hff-pref-ff-4":
        properties["algorithm"] = "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-4"
        properties["id"][0] = "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-4"
    elif properties["algorithm"] == "gbfs-lazy-delta-kckp-hff-pref-ff-8":
        properties["algorithm"] = "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-8"
        properties["id"][0] = "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-8"
    return properties

def rename_algorithm_plain_kckp(properties):
    """Rename algorithm dynamically during fetching."""
    if properties["algorithm"] == "gbfs-lazy-plain-kckp-hff-pref-ff-1":
        properties["algorithm"] = "tyr-gbfs-lazy-plain-kckp-hff-pref-ff-1"
        properties["id"][0] = "tyr-gbfs-lazy-plain-kckp-hff-pref-ff-1"
    return properties

def rename_algorithm_delta_kckp_inner(properties):
    """Rename algorithm dynamically during fetching."""
    if properties["algorithm"] == "gbfs-lazy-delta-kckp-hff-pref-ff-8":
        properties["algorithm"] = "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-8-2"
        properties["id"][0] = "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-8-2"
    return properties

exp.add_fetcher("results_raw/2026-1-8-gbfs_lazy-delta-kckp-1-eval", filter=rename_algorithm_delta_kckp)
exp.add_fetcher("results_raw/2026-1-8-gbfs_lazy-delta-kckp-2-eval", filter=rename_algorithm_delta_kckp)
exp.add_fetcher("results_raw/2026-1-8-gbfs_lazy-delta-kckp-4-eval", filter=rename_algorithm_delta_kckp)
exp.add_fetcher("results_raw/2026-1-8-gbfs_lazy-delta-kckp-8-eval", filter=rename_algorithm_delta_kckp)
exp.add_fetcher("results_raw/2026-1-8-gbfs_lazy-delta-kckp-inner-8-eval", filter=rename_algorithm_delta_kckp_inner) 
exp.add_fetcher("results_raw/2026-1-8-gbfs_lazy-plain-kckp-1-eval", filter=rename_algorithm_plain_kckp)

exp.add_report(BaseReport(attributes=ATTRIBUTES,filter_algorithm=["tyr-gbfs-lazy-plain-kckp-hff-pref-ff-1", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-1", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-2", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-4", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-8", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-8-2"]))

exp.run_steps()
