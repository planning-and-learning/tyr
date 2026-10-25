# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Lab parser for the search statistics that the planner prints."""

import math
import re
from collections.abc import MutableMapping
from typing import Any

from lab import tools
from lab.parser import Parser
from lab.reports import Attribute, arithmetic_mean, geometric_mean

FLOAT = r"([-+]?(?:(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?|inf))"
WORKER_STATISTICS = re.compile(
    r"^\[Search\] Worker (\d+): idle=(\d+) ns, expanded=(\d+), generated=(\d+), "
    r"deadends=(\d+), pruned=(\d+), registered=(\d+), state_storage=(\d+) bytes$",
    re.MULTILINE,
)
WORKER_DESTINATION_LOCK_STATISTICS = re.compile(
    r"^\[Search\] Worker (\d+) destination lock: acquisitions=(\d+), wait=(\d+) ns, hold=(\d+) ns$",
    re.MULTILINE,
)
WORKER_COMMUNICATION_STATISTICS = re.compile(
    r"^\[Search\] Worker (\d+) communication: generated_candidates=(\d+), transferred_candidates=(\d+)$",
    re.MULTILINE,
)

# Lab run properties: a mapping from attribute names to the untyped values Lab stores.
# Lab run properties: values of whatever type each pattern or function stored.
Props = MutableMapping[str, Any]
WorkerRows = list[tuple[int, ...]]


def parse_worker_rows(pattern: re.Pattern[str], content: str, props: Props) -> tuple[WorkerRows, int, list[int] | None]:
    """Parse the per-worker rows matching `pattern`, sorted by worker index.

    Returns:
        The rows, the expected number of workers, and the parsed worker indices if they are not exactly
        0 to the expected number of workers minus one, otherwise None.
    """
    rows = sorted((tuple(map(int, match)) for match in pattern.findall(content)), key=lambda values: values[0])
    expected_workers = props.get("num_search_workers", len(rows))
    indices = [values[0] for values in rows]
    unexpected_indices = indices if indices != list(range(expected_workers)) else None
    return rows, expected_workers, unexpected_indices


def process_invalid(_content: str, props: Props) -> None:
    """Set `invalid` to 1 if the plan was reported invalid, otherwise 0."""
    props["invalid"] = int("invalid" in props)


def process_unsolvable(_content: str, props: Props) -> None:
    """Set `unsolvable` to 1 if the task was reported unsolvable, otherwise 0."""
    props["unsolvable"] = int("unsolvable" in props)


def add_search_time_s(_content: str, props: Props) -> None:
    """Convert the search time to seconds."""
    if "search_time_ns" in props:
        props["search_time_s"] = props["search_time_ns"] / 1_000_000_000


def add_idle_time_s(_content: str, props: Props) -> None:
    """Convert the idle worker time to seconds."""
    if "idle_time_ns" in props:
        props["idle_time_s"] = props["idle_time_ns"] / 1_000_000_000


def parse_worker_statistics(content: str, props: Props) -> None:
    """Store the per-worker search statistics as lists and derive the worker utilizations."""
    matches, _, unexpected_indices = parse_worker_rows(WORKER_STATISTICS, content, props)
    if not matches:
        if props.get("num_search_workers", 0) and any(
            name in props
            for name in ("search_time_ns", "num_expanded", "num_generated_successors", "num_deadends", "num_pruned")
        ):
            tools.add_unexplained_error(props, "Unexpected search worker indices: []")
        return

    if unexpected_indices is not None:
        tools.add_unexplained_error(props, f"Unexpected search worker indices: {unexpected_indices}")
        return

    names = (
        "worker_idle_time_ns",
        "worker_num_expanded",
        "worker_num_generated_successors",
        "worker_num_deadends",
        "worker_num_pruned",
        "worker_num_registered_states",
        "worker_state_storage_memory_usage_bytes",
    )
    for offset, name in enumerate(names, start=1):
        props[name] = [values[offset] for values in matches]

    if props.get("search_time_ns", 0) > 0:
        props["worker_utilizations"] = [
            max(0.0, min(1.0, 1.0 - idle / props["search_time_ns"])) for idle in props["worker_idle_time_ns"]
        ]


def store_worker_communication(matches: WorkerRows, aggregate_names: tuple[str, str], props: Props) -> bool:
    """Store the per-worker candidate counts and check them against the aggregates.

    Returns:
        False if the worker rows are inconsistent, so that no further statistics should be derived.
    """
    generated_candidates = [values[1] for values in matches]
    transferred_candidates = [values[2] for values in matches]
    if any(
        transferred_count > generated_count
        for generated_count, transferred_count in zip(generated_candidates, transferred_candidates, strict=True)
    ):
        tools.add_unexplained_error(props, "Transferred candidates exceed generated candidates for a worker")
        return False

    props["worker_num_generated_candidates"] = generated_candidates
    props["worker_num_transferred_candidates"] = transferred_candidates
    props["worker_communication_overheads"] = [
        transferred_count / generated_count if generated_count else 0.0
        for generated_count, transferred_count in zip(generated_candidates, transferred_candidates, strict=True)
    ]

    totals = (sum(generated_candidates), sum(transferred_candidates))
    for name, total in zip(aggregate_names, totals, strict=True):
        if name in props and props[name] != total:
            tools.add_unexplained_error(props, f"Communication aggregate does not match worker values: {name}")
        else:
            props[name] = total
    return True


def parse_communication_statistics(content: str, props: Props) -> None:
    """Store the per-worker candidate counts and derive the communication overhead."""
    matches, expected_workers, unexpected_indices = parse_worker_rows(WORKER_COMMUNICATION_STATISTICS, content, props)
    aggregate_names = ("num_generated_candidates", "num_transferred_candidates")
    if expected_workers and not matches and any(name in props for name in aggregate_names):
        tools.add_unexplained_error(props, "Missing communication worker statistics")
        return

    if matches:
        if unexpected_indices is not None:
            tools.add_unexplained_error(props, f"Unexpected communication worker indices: {unexpected_indices}")
            return
        if not store_worker_communication(matches, aggregate_names, props):
            return

    present_aggregates = [name in props for name in aggregate_names]
    if any(present_aggregates) and not all(present_aggregates):
        tools.add_unexplained_error(props, "Incomplete communication aggregate statistics")
        return
    if not all(present_aggregates):
        return

    generated_candidates = props["num_generated_candidates"]
    transferred_candidates = props["num_transferred_candidates"]
    if transferred_candidates > generated_candidates:
        tools.add_unexplained_error(props, "Transferred candidates exceed generated candidates")
        return

    overhead = transferred_candidates / generated_candidates if generated_candidates else 0.0
    if "communication_overhead" in props and not math.isclose(
        props["communication_overhead"], overhead, rel_tol=1e-9, abs_tol=1e-12
    ):
        tools.add_unexplained_error(
            props, "Communication overhead does not match generated and transferred candidate counts"
        )
    props["communication_overhead"] = overhead


DESTINATION_LOCK_AGGREGATE_NAMES = (
    "num_destination_lock_acquisitions",
    "destination_lock_wait_time_ns",
    "destination_lock_hold_time_ns",
)


def store_worker_destination_locks(matches: WorkerRows, props: Props) -> bool:
    """Store the per-worker destination-lock statistics and check them against the aggregates.

    Returns:
        False if the aggregates are incomplete, so that no further statistics should be derived.
    """
    names = (
        "worker_destination_lock_acquisitions",
        "worker_destination_lock_wait_time_ns",
        "worker_destination_lock_hold_time_ns",
    )
    for offset, name in enumerate(names, start=1):
        props[name] = [values[offset] for values in matches]

    present_aggregates = [name in props for name in DESTINATION_LOCK_AGGREGATE_NAMES]
    if any(present_aggregates) and not all(present_aggregates):
        tools.add_unexplained_error(props, "Incomplete destination-lock aggregate statistics")
        return False

    for offset, name in enumerate(DESTINATION_LOCK_AGGREGATE_NAMES, start=1):
        total = sum(values[offset] for values in matches)
        if name in props and props[name] != total:
            tools.add_unexplained_error(props, f"Destination-lock aggregate does not match worker values: {name}")
        else:
            props[name] = total
    return True


def add_destination_lock_ratios(props: Props, acquisitions: int) -> None:
    """Derive the mean lock times and the lock times relative to the total worker capacity."""
    wait_time = props.get("destination_lock_wait_time_ns", 0)
    hold_time = props.get("destination_lock_hold_time_ns", 0)
    props["destination_lock_mean_wait_time_ns"] = wait_time / acquisitions if acquisitions else 0.0
    props["destination_lock_mean_hold_time_ns"] = hold_time / acquisitions if acquisitions else 0.0

    capacity = props.get("search_time_ns", 0) * props.get("num_search_workers", 0)
    if capacity > 0:
        props["destination_lock_wait_capacity_ratio"] = wait_time / capacity
        if props.get("collect_destination_lock_statistics"):
            props["communication_efficiency"] = max(0.0, min(1.0, 1.0 - wait_time / capacity))
        props["destination_lock_hold_capacity_ratio"] = hold_time / capacity
        idle_time = props.get("idle_time_ns", 0)
        if idle_time + wait_time > capacity:
            tools.add_unexplained_error(props, "Idle and destination-lock wait time exceed worker capacity")
        props["worker_utilization_excluding_destination_lock_wait"] = max(
            0.0, min(1.0, 1.0 - (idle_time + wait_time) / capacity)
        )


def parse_destination_lock_statistics(content: str, props: Props) -> None:
    """Store the per-worker destination-lock statistics and derive the lock time ratios.

    Disabled collection and interrupted runs drop the aggregates instead.
    """
    matches, expected_workers, unexpected_indices = parse_worker_rows(
        WORKER_DESTINATION_LOCK_STATISTICS, content, props
    )
    if props.get("collect_destination_lock_statistics") == 0 or props.get("out_of_time") or props.get("out_of_memory"):
        for name in DESTINATION_LOCK_AGGREGATE_NAMES:
            props.pop(name, None)
        return

    if matches:
        if unexpected_indices is not None:
            tools.add_unexplained_error(props, f"Unexpected destination-lock worker indices: {unexpected_indices}")
            return
        if not store_worker_destination_locks(matches, props):
            return
    elif expected_workers and (
        props.get("collect_destination_lock_statistics")
        or any(name in props for name in DESTINATION_LOCK_AGGREGATE_NAMES)
    ):
        tools.add_unexplained_error(props, "Missing destination-lock worker statistics")

    acquisitions = props.get("num_destination_lock_acquisitions")
    if acquisitions is not None:
        add_destination_lock_ratios(props, acquisitions)


def add_total_time_s(_content: str, props: Props) -> None:
    """Convert the total time to seconds."""
    if "total_time_ns" in props:
        props["total_time_s"] = props["total_time_ns"] / 1_000_000_000


def add_preprocessing_time_s(_content: str, props: Props) -> None:
    """Derive the time spent before the search, in seconds."""
    if "total_time_ns" in props and "search_time_ns" in props:
        props["preprocessing_time_s"] = (props["total_time_ns"] - props["search_time_ns"]) / 1_000_000_000


def add_search_time_ms_per_expanded(_content: str, props: Props) -> None:
    """Derive the search time per expanded state, in milliseconds."""
    if "search_time_ns" in props and "num_expanded" in props and props["num_expanded"] > 0:
        props["search_time_ms_per_expanded"] = props["search_time_ns"] / 1_000_000 / props["num_expanded"]


def add_memory_mb(_content: str, props: Props) -> None:
    """Convert the peak memory usage to megabytes."""
    if "peak_memory_usage_bytes" in props:
        props["memory_mb"] = props["peak_memory_usage_bytes"] / 1_000_000


def add_coverage(_content: str, props: Props) -> None:
    """Set `coverage` to 1 if the task was solved or proven unsolvable, otherwise 0."""
    if "length" in props or props.get("unsolvable", 0):
        props["coverage"] = 1
    else:
        props["coverage"] = 0


def add_out_of_memory(content: str, props: Props) -> None:
    """Set `out_of_memory` to 1 if the error log reports a failed allocation, otherwise 0."""
    props["out_of_memory"] = int("std::bad_alloc" in content)


def add_out_of_time(content: str, props: Props) -> None:
    """Set `out_of_time` to 1 if the driver log reports an exceeded time limit, otherwise 0."""
    props["out_of_time"] = int(bool(re.search(r"exceeded (?:CPU|wall-clock) time limit:", content)))


# Each Lab pattern: the property name, the regex capturing its value, and the type of the value.
SEARCH_PATTERNS: tuple[tuple[str, str, type[float | int | str]], ...] = (
    ("cost", rf"\[.*\] Plan cost: {FLOAT}", float),
    ("length", r"\[.*\] Plan length: (\d+)", int),
    ("initial_h_value", rf"\[.*\] Start node h_value: {FLOAT}", float),
    ("initial_f_value", rf"\[.*\] Start node f_value: {FLOAT}", float),
    ("num_datalog_threads", r"\[INPUT\] Num Datalog threads: (\d+)", int),
    ("num_search_workers", r"\[INPUT\] Num search workers: (\d+)", int),
    ("state_repository_mode", r"\[INPUT\] State repository mode: (hash-distributed|shared)", str),
    ("dist_hash_mode", r"\[INPUT\] Distribution hash mode: (random|lmcut)", str),
    ("parallel_search_mode", r"\[INPUT\] Parallel search mode: (synchronous|asynchronous)", str),
    ("collect_destination_lock_statistics", r"\[INPUT\] Collect destination lock statistics: ([01])", int),
    ("search_time_ms", r"\[Search\] Search time: (\d+) ms", int),
    ("search_time_ns", r"\[Search\] Search time: \d+ ms \((\d+) ns\)", int),
    ("idle_time_ms", r"\[Search\] Idle worker time: (\d+) ms", int),
    ("idle_time_ns", r"\[Search\] Idle worker time: \d+ ms \((\d+) ns\)", int),
    ("worker_utilization", rf"\[Search\] Worker utilization: {FLOAT}", float),
    ("num_destination_lock_acquisitions", r"\[Search\] Destination lock acquisitions: (\d+)", int),
    ("destination_lock_wait_time_ns", r"\[Search\] Destination lock wait time: \d+ ms \((\d+) ns\)", int),
    ("destination_lock_hold_time_ns", r"\[Search\] Destination lock hold time: \d+ ms \((\d+) ns\)", int),
    ("num_expanded", r"\[Search\] Number of expanded states: (\d+)", int),
    ("num_generated_successors", r"\[Search\] Number of generated successors: (\d+)", int),
    ("num_deadends", r"\[Search\] Number of dead-end states: (\d+)", int),
    ("num_pruned", r"\[Search\] Number of pruned states: (\d+)", int),
    ("num_generated_candidates", r"\[Search\] Number of generated candidates: (\d+)", int),
    ("num_transferred_candidates", r"\[Search\] Number of transferred candidates: (\d+)", int),
    ("communication_overhead", rf"\[Search\] Communication overhead: {FLOAT}", float),
    ("num_registered_states", r"\[Search\] Number of registered states: (\d+)", int),
    ("search_state_storage_memory_usage_bytes", r"\[Search\] State storage memory usage: (\d+) bytes", int),
    ("search_action_bindings_memory_usage_bytes", r"\[Search\] Action bindings memory usage: (\d+) bytes", int),
    ("search_predicate_bindings_memory_usage_bytes", r"\[Search\] Predicate bindings memory usage: (\d+) bytes", int),
    ("search_axiom_bindings_memory_usage_bytes", r"\[Search\] Axiom bindings memory usage: (\d+) bytes", int),
    ("search_function_bindings_memory_usage_bytes", r"\[Search\] Function bindings memory usage: (\d+) bytes", int),
    ("num_expanded_until_last_snapshot", r"\[Search\] Number of expanded states at last snapshot: (\d+)", int),
    (
        "num_generated_successors_until_last_snapshot",
        r"\[Search\] Number of generated successors at last snapshot: (\d+)",
        int,
    ),
    (
        "num_generated_candidates_until_last_snapshot",
        r"\[Search\] Number of generated candidates at last snapshot: (\d+)",
        int,
    ),
    (
        "num_transferred_candidates_until_last_snapshot",
        r"\[Search\] Number of transferred candidates at last snapshot: (\d+)",
        int,
    ),
    ("num_deadends_until_last_snapshot", r"\[Search\] Number of deadend states at last snapshot: (\d+)", int),
    ("num_pruned_until_last_snapshot", r"\[Search\] Number of pruned states at last snapshot: (\d+)", int),
    ("total_time_ms", r"\[Total\] Total time: (\d+) ms", int),
    ("total_time_ns", r"\[Total\] Total time: \d+ ms \((\d+) ns\)", int),
    ("num_fluent_atoms", r"\[Total\] Number of fluent atoms: (\d+)", int),
    ("num_derived_atoms", r"\[Total\] Number of derived atoms: (\d+)", int),
    ("num_fluent_fterms", r"\[Total\] Number of fluent fterms: (\d+)", int),
    ("action_bindings_memory_usage_bytes", r"\[Total\] Action bindings memory usage: (\d+) bytes", int),
    ("predicate_bindings_memory_usage_bytes", r"\[Total\] Predicate bindings memory usage: (\d+) bytes", int),
    ("axiom_bindings_memory_usage_bytes", r"\[Total\] Axiom bindings memory usage: (\d+) bytes", int),
    ("function_bindings_memory_usage_bytes", r"\[Total\] Function bindings memory usage: (\d+) bytes", int),
    ("states_memory_usage_bytes", r"\[Total\] States memory usage: (\d+) bytes", int),
    ("retained_plan_states_memory_usage_bytes", r"\[Total\] Retained plan states memory usage: (\d+) bytes", int),
    ("peak_memory_usage_bytes", r"\[Total\] Peak memory usage: (\d+) bytes", int),
    ("unsolvable", r"(Task is unsolvable!)", str),
    ("invalid", r"(Plan invalid)", str),
    ("num_objects", r"\[Total\] Number of objects: (\d+)", int),
)


class SearchParser(Parser):
    """Parse the search statistics of the planner log into Lab run properties.

    Example log excerpt::

        [Total] Number of objects: 4
        [GBFS] Search started.
        [GBFS] Start node h_value: 3
        [GBFS] New best h_value: 2 with num expanded states 3 and num generated successors 5 (0 ms)
        [GBFS] New best h_value: 1 with num expanded states 4 and num generated successors 7 (0 ms)
        [GBFS] Search ended.
        [Search] Search time: 0 ms (743179 ns)
        [Search] Number of expanded states: 4
        [Search] Number of generated successors: 7
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

    def __init__(self) -> None:
        """Register the search patterns and the functions deriving further properties."""
        super().__init__()
        for name, regex, value_type in SEARCH_PATTERNS:
            self.add_pattern(name, regex, "run.log", value_type)

        self.add_function(process_invalid)
        self.add_function(process_unsolvable)
        self.add_function(add_search_time_s)
        self.add_function(add_idle_time_s)
        self.add_function(add_out_of_memory, "run.err")
        self.add_function(add_out_of_time, "driver.log")
        self.add_function(parse_worker_statistics)
        self.add_function(parse_communication_statistics)
        self.add_function(parse_destination_lock_statistics)
        self.add_function(add_total_time_s)
        self.add_function(add_preprocessing_time_s)
        self.add_function(add_search_time_ms_per_expanded)
        self.add_function(add_memory_mb)
        self.add_function(add_coverage)

    @staticmethod
    def get_attributes() -> list[str | Attribute]:
        """Return the report attributes this parser produces."""
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
            "num_datalog_threads",
            "num_search_workers",
            "state_repository_mode",
            "dist_hash_mode",
            "parallel_search_mode",
            "collect_destination_lock_statistics",
            Attribute("search_time_s", function=geometric_mean, digits=2),
            Attribute("idle_time_s", function=arithmetic_mean, digits=2),
            Attribute("worker_utilization", function=arithmetic_mean, min_wins=False, digits=3),
            "num_destination_lock_acquisitions",
            Attribute("destination_lock_wait_time_ns", function=arithmetic_mean, digits=2),
            Attribute("destination_lock_hold_time_ns", function=arithmetic_mean, digits=2),
            Attribute("destination_lock_mean_wait_time_ns", function=arithmetic_mean, digits=2),
            Attribute("destination_lock_mean_hold_time_ns", function=arithmetic_mean, digits=2),
            Attribute("destination_lock_wait_capacity_ratio", function=arithmetic_mean, digits=3),
            Attribute("communication_efficiency", function=arithmetic_mean, min_wins=False, digits=3),
            Attribute("destination_lock_hold_capacity_ratio", function=arithmetic_mean, digits=3),
            Attribute(
                "worker_utilization_excluding_destination_lock_wait", function=arithmetic_mean, min_wins=False, digits=3
            ),
            "num_expanded",
            "num_generated_successors",
            "num_deadends",
            "num_pruned",
            "num_generated_candidates",
            "num_transferred_candidates",
            Attribute("communication_overhead", function=arithmetic_mean, digits=3),
            "num_registered_states",
            "num_expanded_until_last_snapshot",
            "num_generated_successors_until_last_snapshot",
            "num_generated_candidates_until_last_snapshot",
            "num_transferred_candidates_until_last_snapshot",
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
