# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Lab parser for the LM-cut statistics that the planner prints.

Example log excerpt::

    [LMCut] Number of evaluations: 413
    [LMCut] Number of hmax runs: 9647
    [LMCut] Hmax time: 32575 ms (32575100000 ns)
    [LMCut] Goal zone time: 1924 ms (1924190000 ns)
    [LMCut] Before goal zone time: 23450 ms (23450500000 ns)
    [LMCut] Cut time: 2074 ms (2074170000 ns)
"""

from collections.abc import MutableMapping
from typing import Any

from lab.parser import Parser
from lab.reports import Attribute, arithmetic_mean, geometric_mean

# The parts of an evaluation: the name in the log and in the property names.
PARTS = (("Hmax", "hmax"), ("Goal zone", "goal_zone"), ("Before goal zone", "before_goal_zone"), ("Cut", "cut"))


def add_derived(_content: str, props: MutableMapping[str, Any]) -> None:
    """Derive the total time, the per-evaluation averages, and each part's share of the total time."""
    times = [props.get(f"lmcut_{part}_time_ns") for _, part in PARTS]
    evaluations = props.get("lmcut_num_evaluations")
    if None in times or not evaluations:
        return
    total = sum(times)
    props["lmcut_time_s"] = total / 1_000_000_000
    props["lmcut_time_ms_per_evaluation"] = total / 1_000_000 / evaluations
    props["lmcut_hmax_runs_per_evaluation"] = props["lmcut_num_hmax_runs"] / evaluations
    if total > 0:
        for (_, part), time in zip(PARTS, times, strict=True):
            props[f"lmcut_{part}_time_share"] = time / total


class LMCutParser(Parser):
    """Parse the LM-cut statistics of the planner log into Lab run properties."""

    def __init__(self) -> None:
        """Register the LM-cut patterns and the function deriving further properties."""
        super().__init__()
        self.add_pattern("lmcut_num_evaluations", r"\[LMCut\] Number of evaluations: (\d+)", "run.log", int)
        self.add_pattern("lmcut_num_hmax_runs", r"\[LMCut\] Number of hmax runs: (\d+)", "run.log", int)
        for name, part in PARTS:
            self.add_pattern(f"lmcut_{part}_time_ns", rf"\[LMCut\] {name} time: \d+ ms \((\d+) ns\)", "run.log", int)
        self.add_function(add_derived)

    @staticmethod
    def get_attributes() -> list[str | Attribute]:
        """Return the report attributes this parser produces."""
        return [
            "lmcut_num_evaluations",
            "lmcut_num_hmax_runs",
            Attribute("lmcut_hmax_runs_per_evaluation", function=geometric_mean, digits=2),
            Attribute("lmcut_time_s", function=geometric_mean, digits=2),
            Attribute("lmcut_time_ms_per_evaluation", function=geometric_mean, digits=3),
            *(Attribute(f"lmcut_{part}_time_share", function=arithmetic_mean, digits=3) for _, part in PARTS),
        ]
