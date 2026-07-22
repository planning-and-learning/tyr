#!/usr/bin/env python3
"""Regenerate the astar (eager) search-statistics fixtures."""

from __future__ import annotations

from functools import partial
from pathlib import Path

from .fixture_generation import COST_SUFFIXES, HEURISTICS, ROOT, ConfigSpec, TaskKind, generate_main, run_search_config

FIXTURES: dict[TaskKind, Path] = {
    "ground": ROOT / "tests/fixtures/planning/algorithms/ground/astar.json",
    "lifted": ROOT / "tests/fixtures/planning/algorithms/lifted/astar.json",
}
CONFIGS: list[ConfigSpec] = [(heuristic, suffix) for heuristic in HEURISTICS for suffix in COST_SUFFIXES]

if __name__ == "__main__":
    generate_main(Path(__file__).resolve(), FIXTURES, CONFIGS, partial(run_search_config, "astar_eager"))
