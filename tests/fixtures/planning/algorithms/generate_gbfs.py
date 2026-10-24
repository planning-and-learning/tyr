# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Regenerate the gbfs (lazy) search-statistics fixtures."""

from __future__ import annotations

from functools import partial
from pathlib import Path

from tests.fixtures.planning.algorithms.fixture_generation import (
    ROOT,
    SEARCH_CONFIGS,
    TaskKind,
    generate_main,
    run_search_config,
)

FIXTURES: dict[TaskKind, Path] = {
    "ground": ROOT / "tests/fixtures/planning/algorithms/ground/gbfs.json",
    "lifted": ROOT / "tests/fixtures/planning/algorithms/lifted/gbfs.json",
}

if __name__ == "__main__":
    generate_main(Path(__file__).resolve(), FIXTURES, SEARCH_CONFIGS, partial(run_search_config, "gbfs_lazy"))
