# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Lab parser for the search statistics that Powerlifted prints, under the property names of the search parser.

Example log excerpt::

    Initial heuristic value 10
    Total time: 0.000376
    Total plan cost: 10
    Plan length: 2 step(s).
    Expanded 3 state(s).
    Generated 7 state(s).
    Peak memory usage: 8040 kB
"""

from collections.abc import MutableMapping
from typing import Any

from lab.parser import Parser
from lab.reports import Attribute, geometric_mean

# Run as a script, this directory is on sys.path; imported as `experiments.parser_powerlifted`, the repository root is.
if __package__:
    from experiments.parser_search import FLOAT, add_coverage, add_out_of_memory, add_out_of_time, process_invalid
else:
    from parser_search import FLOAT, add_coverage, add_out_of_memory, add_out_of_time, process_invalid

PATTERNS: tuple[tuple[str, str, type[float | int | str]], ...] = (
    ("cost", rf"Total plan cost: {FLOAT}", float),
    ("length", r"Plan length: (\d+) step", int),
    ("initial_h_value", rf"Initial heuristic value {FLOAT}", float),
    ("search_time_s", rf"Total time: {FLOAT}", float),
    ("num_expanded", r"Expanded (\d+) state", int),
    ("num_generated_successors", r"Generated (\d+) state", int),
    ("peak_memory_usage_kb", r"Peak memory usage: (\d+) kB", int),
    ("invalid", r"(Plan invalid)", str),
)


def add_memory_mb(_content: str, props: MutableMapping[str, Any]) -> None:
    """Convert the peak memory usage to megabytes."""
    if "peak_memory_usage_kb" in props:
        props["memory_mb"] = props["peak_memory_usage_kb"] / 1000


class PowerliftedParser(Parser):
    """Parse the search statistics of the Powerlifted log into Lab run properties."""

    def __init__(self) -> None:
        """Register the patterns and the functions deriving further properties."""
        super().__init__()
        for name, regex, value_type in PATTERNS:
            self.add_pattern(name, regex, "run.log", value_type)
        self.add_function(process_invalid)
        self.add_function(add_out_of_memory, "run.err")
        self.add_function(add_out_of_time, "driver.log")
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
            "invalid",
            "initial_h_value",
            "num_expanded",
            "num_generated_successors",
            Attribute("search_time_s", function=geometric_mean, digits=2),
            Attribute("memory_mb", function=geometric_mean),
        ]
