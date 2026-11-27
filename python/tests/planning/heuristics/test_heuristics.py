# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from pytyr import planning


def test_heuristics_without_a_relaxed_plan_return_none() -> None:
    for task_module in (planning.ground, planning.lifted):
        assert task_module.BlindHeuristic().get_relaxed_plan() is None
