#! /usr/bin/env python

import platform
import re
import sys

from pathlib import Path

import pypddl_datasets

from downward.reports.absolute import AbsoluteReport
from lab.environments import TetralithEnvironment, LocalEnvironment
from lab.experiment import Experiment
from lab.reports import Attribute, geometric_mean, arithmetic_mean

DIR = Path(__file__).resolve().parent.parent
REPO = DIR.parent.parent

sys.path.append(str(REPO))

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



NODE = platform.node()
REMOTE = re.match(r"tetralith\d+.nsc.liu.se|n\d+", NODE)

NUM_THREADS = 2
RANDOM_SEED = 0

if REMOTE:
    ENV = TetralithEnvironment(
        setup=TetralithEnvironment.DEFAULT_SETUP,
        memory_per_cpu="2840M",
        cpus_per_task=6,  # 6*2840 >= 16000
        extra_options="#SBATCH --account=naiss2025-5-382")
    
else:
    ENV = LocalEnvironment(processes=6)

if REMOTE:
    SUITES = [
        #"cnot-synthesis",
        #"ipc-optimal-strips",
        #"ipc-optimal-adl",
        #"ipc-satisficing-strips",
        #"ipc-satisficing-adl",
        #"ipc2023-learning",
        #"autoscale-optimal-strips",
        "autoscale-agile-strips",
        "htg",
        #"pushworld",
        #"beluga2025-scalability-deterministic",
        #"mine-pddl-classical",
    ]
    WALL_TIME_LIMIT = 10 * 60
else:
    SUITES = [
        #"cnot-synthesis-test",
        #"ipc-optimal-strips-test",
        #"ipc-optimal-adl-test",
        "ipc-satisficing-strips-test",
        "ipc-satisficing-adl-test",
        #"ipc2023-learning-test",
        #"autoscale-optimal-strips-test",
        "autoscale-agile-strips-test",
        #"htg-test",
        #"pushworld-test",
        #"beluga2025-scalability-deterministic-test",
        #"mine-pddl-classical-test",
    ]
    WALL_TIME_LIMIT = 1

ATTRIBUTES = [
    "run_dir",
]
ATTRIBUTES += SearchParser.get_attributes()
ATTRIBUTES += DatalogParser.get_attributes()

MEMORY_LIMIT = 16000

# Create a new experiment.
exp = Experiment(environment=ENV)
exp.add_parser(SearchParser())
exp.add_parser(DatalogParser())

PLANNER_DIR = REPO / "build-plain-kpkc" / "exe" / "gbfs_lazy"

exp.add_resource("planner_exe", PLANNER_DIR)
exp.add_resource("run_planner", DIR / "gbfs_lazy.sh")

base_cmd = [
    "{run_planner}",
    "{planner_exe}",
    "{domain}",
    "{problem}",
    "plan.out",
    "rpg_ff",
    str(NUM_THREADS),
    str(RANDOM_SEED),
]

for SUITE in SUITES:
    for domain in pypddl_datasets.fetch_suite(SUITE).domains:
        for task in domain.tasks:
            run = exp.add_run()
            run.add_resource("domain", task.domain_path, symlink=True)
            run.add_resource("problem", task.task_path, symlink=True)

            run.add_command(
                f"gbfs-lazy-plain-kckp-hff-pref-ff-{NUM_THREADS}",
                base_cmd + ["-S"],
                time_limit=None,
                wall_time_limit=WALL_TIME_LIMIT,
                memory_limit=MEMORY_LIMIT,
            )
            # AbsoluteReport needs the following properties:
            # 'domain', 'problem', 'algorithm', 'coverage'.
            run.set_property("domain", task.domain)
            run.set_property("problem", task.problem)
            run.set_property("algorithm", f"gbfs-lazy-plain-kckp-hff-pref-ff-{NUM_THREADS}")
            # BaseReport needs the following properties:
            # 'time_limit', 'memory_limit'.
            run.set_property("wall_time_limit", WALL_TIME_LIMIT)
            run.set_property("memory_limit", MEMORY_LIMIT)
            # Every run has to have a unique id in the form of a list.
            # The algorithm name is only really needed when there are
            # multiple algorithms.
            run.set_property("id", [f"gbfs-lazy-plain-kckp-hff-pref-ff-{NUM_THREADS}", task.domain, task.problem])

# Add step that writes experiment files to disk.
exp.add_step("build", exp.build)

# Add step that executes all runs.
exp.add_step("start", exp.start_runs)

exp.add_step("parse", exp.parse)

# Add step that collects properties from run directories and
# writes them to *-eval/properties.
exp.add_fetcher(name="fetch")

# Make a report.
exp.add_report(BaseReport(attributes=ATTRIBUTES), outfile="report.html")

# Parse the commandline and run the specified steps.
exp.run_steps()
