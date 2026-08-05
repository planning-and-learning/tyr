#! /usr/bin/env python

from lab.parser import Parser
from lab import tools
from lab.reports import Attribute, geometric_mean, arithmetic_mean

import re


FLOAT = r"([-+]?(?:(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?|inf))"
WORKER_STATISTICS = re.compile(
    r"^\[Search\] Worker (\d+): idle=(\d+) ns, expanded=(\d+), generated=(\d+), "
    r"deadends=(\d+), pruned=(\d+), registered=(\d+), state_storage=(\d+) bytes$",
    re.MULTILINE,
)


def process_invalid(content, props):
    props["invalid"] = int("invalid" in props)

def process_unsolvable(content, props):
    props["unsolvable"] = int("unsolvable" in props)

def add_search_time_s(content, props):
    if "search_time_ns" in props:
        props["search_time_s"] = props["search_time_ns"] / 1_000_000_000


def add_idle_time_s(content, props):
    if "idle_time_ns" in props:
        props["idle_time_s"] = props["idle_time_ns"] / 1_000_000_000


def parse_worker_statistics(content, props):
    matches = sorted((tuple(map(int, match)) for match in WORKER_STATISTICS.findall(content)), key=lambda values: values[0])
    if not matches:
        return

    indices = [values[0] for values in matches]
    if indices != list(range(len(matches))):
        tools.add_unexplained_error(props, f"Unexpected search worker indices: {indices}")
        return

    names = (
        "worker_idle_time_ns",
        "worker_num_expanded",
        "worker_num_generated",
        "worker_num_deadends",
        "worker_num_pruned",
        "worker_num_registered_states",
        "worker_state_storage_memory_usage_bytes",
    )
    for offset, name in enumerate(names, start=1):
        props[name] = [values[offset] for values in matches]

    if props.get("search_time_ns", 0) > 0:
        props["worker_utilizations"] = [max(0.0, min(1.0, 1.0 - idle / props["search_time_ns"])) for idle in props["worker_idle_time_ns"]]


def add_total_time_s(content, props):
    if "total_time_ns" in props:
        props["total_time_s"] = props["total_time_ns"] / 1_000_000_000

def add_preprocessing_time_s(content, props):
    if "total_time_ns" in props and "search_time_ns" in props:
        props["preprocessing_time_s"] = (props["total_time_ns"] - props["search_time_ns"]) / 1_000_000_000

def add_search_time_ms_per_expanded(context, props):
    if "search_time_ns" in props and "num_expanded" in props:
        if props["num_expanded"] > 0:
            props["search_time_ms_per_expanded"] = props["search_time_ns"] / 1_000_000 / props["num_expanded"]

def add_memory_mb(content, props):
    if "peak_memory_usage_bytes" in props:
        props["memory_mb"] = props["peak_memory_usage_bytes"] / 1_000_000

def add_coverage(content, props):
    if "length" in props or props.get("unsolvable", 0):
        props["coverage"] = 1
    else:
        props["coverage"] = 0

def add_out_of_memory(content, props):
    props["out_of_memory"] = int("std::bad_alloc" in content)

def add_out_of_time(content, props):
    props["out_of_time"] = int(bool(re.search(r"exceeded (?:CPU|wall-clock) time limit:", content)))

class SearchParser(Parser):
    """
    [Total] Number of objects: 4
    [GBFS] Search started.
    [GBFS] Start node h_value: 3
    [GBFS] New best h_value: 2 with num expanded states 3 and num generated states 5 (0 ms)
    [GBFS] New best h_value: 1 with num expanded states 4 and num generated states 7 (0 ms)
    [GBFS] Search ended.
    [Search] Search time: 0 ms (743179 ns)
    [Search] Number of expanded states: 4
    [Search] Number of generated states: 7
    [Search] Number of pruned states: 0
    [GBFS] Plan found.
    [GBFS] Plan cost: 3
    [GBFS] Plan length: 3
    (pick ball2 rooma left)
    (move rooma roomb)
    (drop ball2 roomb left)

    ...

    [Total] Peak memory usage: 513306624 bytes
    [Total] Total time: 4 ms (4424855 ns)

    """
    def __init__(self):
        super().__init__()
        self.add_pattern("cost", rf"\[.*\] Plan cost: {FLOAT}", type=float)
        self.add_pattern("length", r"\[.*\] Plan length: (\d+)", type=int)
        self.add_pattern("initial_h_value", rf"\[.*\] Start node h_value: {FLOAT}", type=float)
        self.add_pattern("initial_f_value", rf"\[.*\] Start node f_value: {FLOAT}", type=float)

        self.add_pattern("num_worker_threads", r"\[INPUT\] Num worker threads: (\d+)", type=int)
        self.add_pattern("num_search_workers", r"\[INPUT\] Num search workers: (\d+)", type=int)
        self.add_pattern(
            "state_repository_mode",
            r"\[INPUT\] State repository mode: (hash-distributed|shared)",
            type=str,
        )
        self.add_pattern("parallel_search_mode", r"\[INPUT\] Parallel search mode: (synchronous|asynchronous)", type=str)

        self.add_pattern("search_time_ms", r"\[Search\] Search time: (\d+) ms", type=int)
        self.add_pattern("search_time_ns", r"\[Search\] Search time: \d+ ms \((\d+) ns\)", type=int)
        self.add_pattern("idle_time_ms", r"\[Search\] Idle worker time: (\d+) ms", type=int)
        self.add_pattern("idle_time_ns", r"\[Search\] Idle worker time: \d+ ms \((\d+) ns\)", type=int)
        self.add_pattern("worker_utilization", rf"\[Search\] Worker utilization: {FLOAT}", type=float)
        self.add_pattern("num_expanded", r"\[Search\] Number of expanded states: (\d+)", type=int)
        self.add_pattern("num_generated", r"\[Search\] Number of generated states: (\d+)", type=int)
        self.add_pattern("num_deadends", r"\[Search\] Number of dead-end states: (\d+)", type=int)
        self.add_pattern("num_pruned", r"\[Search\] Number of pruned states: (\d+)", type=int)
        self.add_pattern("num_registered_states", r"\[Search\] Number of registered states: (\d+)", type=int)
        self.add_pattern("search_state_storage_memory_usage_bytes", r"\[Search\] State storage memory usage: (\d+) bytes", type=int)
        self.add_pattern("search_action_bindings_memory_usage_bytes", r"\[Search\] Action bindings memory usage: (\d+) bytes", type=int)
        self.add_pattern("search_predicate_bindings_memory_usage_bytes", r"\[Search\] Predicate bindings memory usage: (\d+) bytes", type=int)
        self.add_pattern("search_axiom_bindings_memory_usage_bytes", r"\[Search\] Axiom bindings memory usage: (\d+) bytes", type=int)
        self.add_pattern("search_function_bindings_memory_usage_bytes", r"\[Search\] Function bindings memory usage: (\d+) bytes", type=int)
        self.add_pattern("num_expanded_until_last_snapshot", r"\[Search\] Number of expanded states at last snapshot: (\d+)", type=int)
        self.add_pattern("num_generated_until_last_snapshot", r"\[Search\] Number of generated states at last snapshot: (\d+)", type=int)
        self.add_pattern("num_deadends_until_last_snapshot", r"\[Search\] Number of deadend states at last snapshot: (\d+)", type=int)
        self.add_pattern("num_pruned_until_last_snapshot", r"\[Search\] Number of pruned states at last snapshot: (\d+)", type=int)

        self.add_pattern("total_time_ms", r"\[Total\] Total time: (\d+) ms", type=int)
        self.add_pattern("total_time_ns", r"\[Total\] Total time: \d+ ms \((\d+) ns\)", type=int)
        self.add_pattern("num_fluent_atoms", r"\[Total\] Number of fluent atoms: (\d+)", type=int)
        self.add_pattern("num_derived_atoms", r"\[Total\] Number of derived atoms: (\d+)", type=int)
        self.add_pattern("num_fluent_fterms", r"\[Total\] Number of fluent fterms: (\d+)", type=int)
        self.add_pattern("action_bindings_memory_usage_bytes", r"\[Total\] Action bindings memory usage: (\d+) bytes", type=int)
        self.add_pattern("predicate_bindings_memory_usage_bytes", r"\[Total\] Predicate bindings memory usage: (\d+) bytes", type=int)
        self.add_pattern("axiom_bindings_memory_usage_bytes", r"\[Total\] Axiom bindings memory usage: (\d+) bytes", type=int)
        self.add_pattern("function_bindings_memory_usage_bytes", r"\[Total\] Function bindings memory usage: (\d+) bytes", type=int)
        self.add_pattern("states_memory_usage_bytes", r"\[Total\] States memory usage: (\d+) bytes", type=int)
        self.add_pattern("retained_plan_states_memory_usage_bytes", r"\[Total\] Retained plan states memory usage: (\d+) bytes", type=int)
        self.add_pattern("peak_memory_usage_bytes", r"\[Total\] Peak memory usage: (\d+) bytes", type=int)

        self.add_pattern("unsolvable", r"(Task is unsolvable!)", type=str)
        self.add_pattern("invalid", r"(Plan invalid)", type=str)

        self.add_pattern("num_objects", r"\[Total\] Number of objects: (\d+)", type=int)
        
        self.add_function(process_invalid)
        self.add_function(process_unsolvable)
        self.add_function(add_search_time_s)
        self.add_function(add_idle_time_s)
        self.add_function(parse_worker_statistics)
        self.add_function(add_total_time_s)
        self.add_function(add_preprocessing_time_s)
        self.add_function(add_search_time_ms_per_expanded)
        self.add_function(add_memory_mb)
        self.add_function(add_coverage)
        self.add_function(add_out_of_memory, "run.err")
        self.add_function(add_out_of_time, "driver.log")

    @staticmethod
    def get_attributes():
        return [
            Attribute("coverage", min_wins=False),
            "out_of_time",
            "out_of_memory",
            "cost",
            "length",
            "unsolvable",
            "invalid",
            "initial_h_value",
            "initial_f_value",
            "num_worker_threads",
            "num_search_workers",
            "state_repository_mode",
            "parallel_search_mode",
            Attribute("search_time_s", function=geometric_mean, digits=2),
            Attribute("idle_time_s", function=arithmetic_mean, digits=2),
            Attribute("worker_utilization", function=arithmetic_mean, min_wins=False, digits=3),
            "num_expanded",
            "num_generated",
            "num_deadends",
            "num_pruned",
            "num_registered_states",
            "num_expanded_until_last_snapshot",
            "num_generated_until_last_snapshot",
            "num_deadends_until_last_snapshot",
            "num_pruned_until_last_snapshot",
            Attribute("search_time_ms_per_expanded", function=geometric_mean, digits=2),
            Attribute("total_time_s", function=geometric_mean, digits=2),
            Attribute("preprocessing_time_s", function=geometric_mean, digits=2),
            "num_fluent_atoms",
            "num_derived_atoms",
            "num_fluent_fterms",
            "num_objects",
            Attribute("action_bindings_memory_usage_bytes", function=geometric_mean),
            Attribute("predicate_bindings_memory_usage_bytes", function=geometric_mean),
            Attribute("axiom_bindings_memory_usage_bytes", function=geometric_mean),
            Attribute("function_bindings_memory_usage_bytes", function=geometric_mean),
            Attribute("states_memory_usage_bytes", function=geometric_mean),
            Attribute("retained_plan_states_memory_usage_bytes", function=geometric_mean),
            Attribute("search_state_storage_memory_usage_bytes", function=geometric_mean),
            Attribute("search_action_bindings_memory_usage_bytes", function=geometric_mean),
            Attribute("search_predicate_bindings_memory_usage_bytes", function=geometric_mean),
            Attribute("search_axiom_bindings_memory_usage_bytes", function=geometric_mean),
            Attribute("search_function_bindings_memory_usage_bytes", function=geometric_mean),
            Attribute("peak_memory_usage_bytes", function=geometric_mean),
            Attribute("memory_mb", function=geometric_mean),
            
        ]
