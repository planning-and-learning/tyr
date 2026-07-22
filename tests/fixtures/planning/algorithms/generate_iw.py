#!/usr/bin/env python3
"""Regenerate the iw search-statistics fixture (ground + lifted).

Each per-kind object records the search outcome (status, plan, solution_arity) plus the
four counters aggregated across width iterations by the iw event handler. The suite-level
``max_arity`` is preserved and used for the runs.

Usage:
    .venv/bin/python -m tests.fixtures.planning.algorithms.generate_iw [CASE ...]
"""

from __future__ import annotations

from pathlib import Path
from typing import Any, Protocol, cast

from .fixture_generation import (
    MAX_NUM_STATES,
    MAX_TIME,
    RECORDED_STATUSES,
    ROOT,
    SEARCH_STATUS_NAMES,
    ConfigResult,
    ConfigSpec,
    CostSuffix,
    FixtureCase,
    HeuristicName,
    SearchResultLike,
    Statistics,
    TaskKind,
    counters_of,
    generate_main,
    make_context,
    plan_of,
    planning_module,
)

FIXTURES: dict[TaskKind, Path] = {
    "ground": ROOT / "tests/fixtures/planning/algorithms/ground/iw.json",
    "lifted": ROOT / "tests/fixtures/planning/algorithms/lifted/iw.json",
}
CONFIGS: list[ConfigSpec] = [(None, None)]


class _IwStatistics(Protocol):
    def get_solution_arity(self) -> int | None: ...


class _IwEventHandler(Protocol):
    def get_statistics(self) -> _IwStatistics: ...
    def get_search_statistics(self) -> Statistics: ...


def run_config(kind: TaskKind,
               heuristic_name: HeuristicName | None,
               cost_suffix: CostSuffix | None,
               domain_file: Path,
               task_file: Path,
               suite: FixtureCase,
               apply_limits: bool) -> ConfigResult | str:
    max_arity = cast(int, suite["max_arity"])
    context = make_context(kind, domain_file, task_file)
    planning: Any = planning_module(context)  # runtime-selected module (see fixture_generation docstring)

    brfs_solver = planning.brfs.Solver()
    brfs_solver.task = context.task
    brfs_solver.successor_generator = context.successor_generator
    brfs_solver.options.event_handler = planning.brfs.DefaultEventHandler()

    handler = cast(_IwEventHandler, planning.iw.DefaultEventHandler())
    options = planning.iw.Options()
    options.event_handler = handler
    if apply_limits:
        options.max_num_states = MAX_NUM_STATES
        options.max_time = MAX_TIME

    result = cast(SearchResultLike, planning.iw.find_solution(brfs_solver, max_arity, options))

    if result.status not in RECORDED_STATUSES:
        return f"status {SEARCH_STATUS_NAMES[result.status]} within {MAX_TIME}s/{MAX_NUM_STATES} states"

    config: ConfigResult = {"status": SEARCH_STATUS_NAMES[result.status]}
    plan = result.plan
    if plan is not None:
        config["plan"] = plan_of(plan)
    solution_arity = handler.get_statistics().get_solution_arity()
    if solution_arity is not None:
        config["solution_arity"] = solution_arity
    config.update(counters_of(handler.get_search_statistics()))
    return config


if __name__ == "__main__":
    generate_main(Path(__file__).resolve(), FIXTURES, CONFIGS, run_config)
