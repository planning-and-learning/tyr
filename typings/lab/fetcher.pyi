# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from pathlib import Path

class Fetcher:
    def __call__(self, src_dir: str | Path, eval_dir: str | Path | None = None, merge: bool | None = None) -> None: ...
