# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from collections.abc import Callable, Sequence
from typing import Self

def arithmetic_mean(values: Sequence[float]) -> float: ...
def geometric_mean(values: Sequence[float]) -> float: ...

class Attribute(str):
    def __new__(
        cls,
        name: str,
        absolute: bool = False,
        min_wins: bool = True,
        function: Callable[[Sequence[float]], float] | None = None,
        functions: Sequence[Callable[[Sequence[float]], float]] | None = None,
        scale: str | None = None,
        digits: int = 2,
    ) -> Self: ...

class Report:
    def __call__(self, eval_dir: str, outfile: str) -> None: ...
