# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from collections.abc import Sequence
from typing import ClassVar

from lab.reports import Attribute, Report

class AbsoluteReport(Report):
    INFO_ATTRIBUTES: ClassVar[list[str]]
    ERROR_ATTRIBUTES: ClassVar[list[str]]
    def __init__(
        self, *, attributes: Sequence[str | Attribute] | None = None, filter_algorithm: Sequence[str] | None = None
    ) -> None: ...
