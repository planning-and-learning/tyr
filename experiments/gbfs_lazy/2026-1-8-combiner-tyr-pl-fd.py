#! /usr/bin/env python

import sys 

from pathlib import Path

from lab.experiment import Experiment
from downward.reports.absolute import AbsoluteReport

from lab.reports import Attribute, geometric_mean

DIR = Path(__file__).resolve().parent
REPO = DIR.parent.parent
RESULTS_RAW = DIR / "results_raw"

def result_path(name):
    return str(RESULTS_RAW / name)

sys.path.append(str(DIR.parent.parent))

from experiments.parser_search import SearchParser

from pypddl_datasets import suites as _suites

def property_domain(name):
    return name.replace("/", "-")

SUITE_IPC_SATISFICING_ADL = [property_domain(name) for name in _suites.SUITE_IPC_SATISFICING_ADL]
SUITE_IPC_SATISFICING_STRIPS = [property_domain(name) for name in _suites.SUITE_IPC_SATISFICING_STRIPS]
SUITE_HTG = [property_domain(name) for name in _suites.SUITE_HTG]
SUITE_AUTOSCALE_AGILE_STRIPS = [property_domain(name) for name in _suites.SUITE_AUTOSCALE_AGILE_STRIPS]
DOMAIN_BASENAMES = {
    property_domain(name): name.rsplit("/", 1)[-1]
    for suite in (_suites.SUITE_HTG, _suites.SUITE_AUTOSCALE_AGILE_STRIPS)
    for name in suite
}

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

    Attribute("coverage", min_wins=False),
    "cost",
    "length",
    "unsolvable",
    "invalid",
    "initial_h_value",
    Attribute("search_time_s", function=geometric_mean, digits=2),
    "num_expanded",
    "num_generated",
    Attribute("search_time_ms_per_expanded", function=geometric_mean, digits=2),
    Attribute("total_time_s", function=geometric_mean, digits=2),
    Attribute("memory_mb", function=geometric_mean),
]


AUTOSCALE_PREFIX = "classical-autoscale-benchmarks-main-21.11-agile-strips-"
UNSUPPORTED_DOMAIN_NAMES = [
    # Powerlifted: Actions with negated preconditions not supported yet
    "data-network-sat18-strips",
    "pathways",
    "snake-sat18-strips",
    "termes-sat18-strips",
    "tetris-sat14-strips",
    "tidybot-sat11-strips",
    "data-network",
    "snake",
    "termes",
    "tetris",
    "tidybot",
    # Powerlifted: Derived predicates are not supported.
    "optical-telegraphs",
    "philosophers",
    "psr-large",
    "psr-middle",
    # Powerlifted: PDDL feature "when" not supported yet.
    "schedule",
    "spider-sat18-strips",
    # Powerlifted: PDDL feature "imply" not supported yet.
    "miconic-fulladl",
    # Powerlifted: PDDL feature "forall" not supported yet.
    "assembly",
    "caldera-sat18-adl",
    "caldera-split-sat18-adl",
    "citycar-sat14-adl",
    "flashfill-sat18-adl",
    "genome-edit-distance-positional",
    "maintenance-sat14-adl",
    "miconic-simpleadl",
    "nurikabe-sat18-adl",
    "openstacks",
    "openstacks-sat08-adl",
    "settlers-sat18-adl",
    "trucks",
    # Powerlifted: assert (not isinstance(arg.type_name, list))
    "storage",
    "zenotravel",
    # Powerlifted: requirements = pddl.Requirements(opt[1:])
    "cavediving-14-adl",
]
UNSUPPORTED_DOMAINS = [AUTOSCALE_PREFIX + name for name in UNSUPPORTED_DOMAIN_NAMES]

def exclude_domains(run, excluded_domains) -> bool:
    domain = run.get("domain")
    return domain not in excluded_domains

def exclude_unsupported_problems(run) -> bool:
    return not (
        run.get("domain") == "classical-htg-domains-genome-edit-distance"
        and run.get("problem", "").endswith("-positional.pddl")
    )

def merge_domain(run, name) -> bool:
    dom = run.get("domain", "")
    if DOMAIN_BASENAMES.get(dom, dom).startswith(name):
        prob = run.get("problem", "")
        run["problem"] = f"{dom}_{prob}"
        run["domain"] = name
        if "id" in run and isinstance(run["id"], list):
            if len(run["id"]) > 1:
                run["id"][1] = name
            if len(run["id"]) > 2:
                run["id"][2] = run["problem"]

def merge_domains(run) -> bool:
    merge_domain(run, "childsnack")
    merge_domain(run, "visitall")
    merge_domain(run, "genome-edit-distance")
    merge_domain(run, "organic-synthesis")
    merge_domain(run, "blocksworld")
    merge_domain(run, "logistics")

    domain = run.get("domain", "")
    basename = DOMAIN_BASENAMES.get(domain, domain)
    if basename != domain:
        run["domain"] = basename
        if "id" in run and isinstance(run["id"], list) and len(run["id"]) > 1:
            run["id"][1] = basename

    return True

def normalize_htg(run):
    if (
        not exclude_unsupported_problems(run)
        or not exclude_domains(run, UNSUPPORTED_DOMAINS + SUITE_AUTOSCALE_AGILE_STRIPS)
    ):
        return False  
    merge_domains(run)
    return True

def normalize_autoscale(run):
    if not exclude_unsupported_problems(run) or not exclude_domains(run, UNSUPPORTED_DOMAINS + SUITE_HTG):
        return False
    merge_domains(run)
    return True

def normalize(run):  
    if not exclude_unsupported_problems(run) or not exclude_domains(run, UNSUPPORTED_DOMAINS):
        return False 
    merge_domains(run)
    return True

# exp = Experiment("2026-1-8-gbfs_lazy-profiling-tyr-pl-combined")
exp_htg = Experiment(str(DIR / "2026-1-8-gbfs_lazy-htg-tyr-pl-fd"))
exp_htg.add_fetcher(result_path("pl-2026-1-9-lazy-gbfs-ff-pref-ff-eval"), merge=False, filter=normalize_htg)
exp_htg.add_fetcher(result_path("tyr-2026-1-8-gbfs_lazy-combined-eval"), merge=True, filter=normalize_htg)
exp_htg.add_fetcher(result_path("fd-2026-2-28-gbfs-lazy-ff-pref-ff-eval"), merge=True, filter=normalize_htg)
exp_htg.add_report(BaseReport(attributes=ATTRIBUTES, filter_algorithm=["downward-gbfs-lazy-hff-pref-ff", "powerlifted-gbfs-lazy-hff-pref-ff", "tyr-gbfs-lazy-plain-kckp-hff-pref-ff-1", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-1", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-2", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-4", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-8", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-8-2"]))
exp_htg.run_steps()

exp_autoscale = Experiment(str(DIR / "2026-1-8-gbfs_lazy-autoscale-tyr-pl-fd"))
exp_autoscale.add_fetcher(result_path("pl-2026-1-9-lazy-gbfs-ff-pref-ff-eval"), merge=False, filter=normalize_autoscale)
exp_autoscale.add_fetcher(result_path("tyr-2026-1-8-gbfs_lazy-combined-eval"), merge=True, filter=normalize_autoscale)
exp_autoscale.add_fetcher(result_path("fd-2026-2-28-gbfs-lazy-ff-pref-ff-eval"), merge=True, filter=normalize_autoscale)
exp_autoscale.add_report(BaseReport(attributes=ATTRIBUTES, filter_algorithm=["downward-gbfs-lazy-hff-pref-ff", "powerlifted-gbfs-lazy-hff-pref-ff", "tyr-gbfs-lazy-plain-kckp-hff-pref-ff-1", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-1", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-2", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-4", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-8", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-8-2"]))
exp_autoscale.run_steps()

exp = Experiment(str(DIR / "2026-1-8-gbfs_lazy-tyr-pl-fd"))
exp.add_fetcher(result_path("pl-2026-1-9-lazy-gbfs-ff-pref-ff-eval"), merge=False, filter=normalize)
exp.add_fetcher(result_path("tyr-2026-1-8-gbfs_lazy-combined-eval"), merge=True, filter=normalize)
exp.add_fetcher(result_path("fd-2026-2-28-gbfs-lazy-ff-pref-ff-eval"), merge=True, filter=normalize)
exp.add_report(BaseReport(attributes=ATTRIBUTES, filter_algorithm=["downward-gbfs-lazy-hff-pref-ff", "powerlifted-gbfs-lazy-hff-pref-ff", "tyr-gbfs-lazy-plain-kckp-hff-pref-ff-1", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-1", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-2", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-4", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-8", "tyr-gbfs-lazy-delta-kckp-hff-pref-ff-8-2"]))
exp.run_steps()
