#!/usr/bin/env python3
"""Regenerate the brfs search-statistics fixtures."""

from __future__ import annotations

from functools import partial
from pathlib import Path

from .fixture_generation import ROOT, ConfigSpec, TaskKind, generate_main, run_search_config

FIXTURES: dict[TaskKind, Path] = {
    "ground": ROOT / "tests/fixtures/planning/algorithms/ground/brfs.json",
    "lifted": ROOT / "tests/fixtures/planning/algorithms/lifted/brfs.json",
}
CONFIGS: list[ConfigSpec] = [(None, None)]

if __name__ == "__main__":
    generate_main(Path(__file__).resolve(), FIXTURES, CONFIGS, partial(run_search_config, "brfs"))
