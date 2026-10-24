# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Lab report for the search experiments."""

from typing import ClassVar

from downward.reports.absolute import AbsoluteReport


class BaseReport(AbsoluteReport):
    """Absolute report that lists the run limits as info and the run's location in error tables."""

    INFO_ATTRIBUTES: ClassVar[list[str]] = ["time_limit", "wall_time_limit", "memory_limit"]
    ERROR_ATTRIBUTES: ClassVar[list[str]] = [
        "domain",
        "problem",
        "algorithm",
        "unexplained_errors",
        "error",
        "node",
    ]
