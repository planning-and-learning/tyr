# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from collections.abc import Callable, MutableMapping
from pathlib import Path

class Parser:
    def __init__(self) -> None: ...
    # Lab names the fourth parameter `type`; the stub takes it by position only.
    def add_pattern(
        self,
        attribute: str,
        regex: str,
        file: str = "run.log",
        value_type: Callable[[str], object] = ...,
        /,
        flags: str = "",
        required: bool = False,
    ) -> None: ...
    def add_function(
        self, function: Callable[[str, MutableMapping[str, object]], object], file: str = "run.log"
    ) -> None: ...
    def parse(self, run_dir: str | Path, props: MutableMapping[str, object]) -> None: ...
