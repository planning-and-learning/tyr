#!/usr/bin/env python3
"""Regenerate the siw search-statistics fixture (ground + lifted).

Each per-kind object records the search outcome (status, plan, maximum_effective_width)
plus the four counters aggregated across subsearches in the search result. The
suite-level ``max_arity`` is preserved and used for the runs.

Usage:
    .venv/bin/python -m tests.fixtures.planning.algorithms.generate_siw [CASE ...]
"""

from __future__ import annotations

from datetime import timedelta
from pathlib import Path
from typing import Any, Protocol, cast

from pytyr.planning import SearchBudget

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
    TaskKind,
    counters_of,
    generate_main,
    make_context,
    plan_of,
    planning_module,
)

FIXTURES: dict[TaskKind, Path] = {
    "ground": ROOT / "tests/fixtures/planning/algorithms/ground/siw.json",
    "lifted": ROOT / "tests/fixtures/planning/algorithms/lifted/siw.json",
}
CONFIGS: list[ConfigSpec] = [(None, None)]


class _SiwStatistics(Protocol):
    def get_maximum_effective_width(self) -> int | None: ...


class _SiwEventHandler(Protocol):
    def get_statistics(self) -> _SiwStatistics: ...


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
    brfs_solver.state_repository = context.state_repository
    brfs_solver.axiom_evaluator = context.axiom_evaluator
    brfs_solver.successor_generator = context.successor_generator
    brfs_solver.options.event_handler = planning.brfs.DefaultEventHandler()
    if apply_limits:
        brfs_solver.options.search_budget = SearchBudget(MAX_NUM_STATES, timedelta(seconds=MAX_TIME))

    iw_solver = planning.iw.Solver()
    iw_solver.brfs_solver = brfs_solver
    iw_solver.max_arity = max_arity

    handler = cast(_SiwEventHandler, planning.siw.DefaultEventHandler())
    options = planning.siw.Options()
    options.event_handler = handler

    result = cast(SearchResultLike, planning.siw.find_solution(iw_solver, options))

    if result.status not in RECORDED_STATUSES:
        return f"status {SEARCH_STATUS_NAMES[result.status]} within {MAX_TIME}s/{MAX_NUM_STATES} states"

    config: ConfigResult = {"status": SEARCH_STATUS_NAMES[result.status]}
    maximum_effective_width = handler.get_statistics().get_maximum_effective_width()
    if maximum_effective_width is not None:
        config["maximum_effective_width"] = maximum_effective_width
    plan = result.plan
    if plan is not None:
        config["plan"] = plan_of(plan)
    config.update(counters_of(result.statistics))
    return config


if __name__ == "__main__":
    generate_main(Path(__file__).resolve(), FIXTURES, CONFIGS, run_config)
