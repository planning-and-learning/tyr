# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import argparse
from collections.abc import Callable
from pathlib import Path
from typing import ParamSpec

from lab.environments import Environment
from lab.parser import Parser
from lab.reports import Report
from lab.steps import Step

_P = ParamSpec("_P")

ARGPARSER: argparse.ArgumentParser

class _Buildable:
    def set_property(self, name: str, value: object) -> None: ...
    def add_resource(self, name: str, source: str | Path, dest: str = "", symlink: bool = False) -> None: ...

class Run(_Buildable):
    def add_command(
        self,
        name: str,
        command: list[str],
        time_limit: int | None = None,
        wall_time_limit: int | None = None,
        memory_limit: int | None = None,
        soft_stdout_limit: int = 1024,
        hard_stdout_limit: int = 10240,
        soft_stderr_limit: int = 64,
        hard_stderr_limit: int = 10240,
    ) -> None: ...

class Experiment(_Buildable):
    steps: list[Step]
    runs: list[Run]
    def __init__(self, path: str | Path | None = None, environment: Environment | None = None) -> None: ...
    def add_step(self, name: str, function: Callable[_P, object], *args: _P.args, **kwargs: _P.kwargs) -> None: ...
    def add_parser(self, parser: Parser) -> None: ...
    def add_fetcher(
        self, src: str | Path | None = None, dest: str | Path | None = None, *, name: str | None = None
    ) -> None: ...
    def add_report(self, report: Report, name: str = "", eval_dir: str = "", outfile: str = "") -> None: ...
    def add_run(self, run: Run | None = None) -> Run: ...
    def build(self, write_to_disk: bool = True) -> None: ...
    def start_runs(self) -> None: ...
    def parse(self) -> None: ...
    def run_steps(self) -> None: ...
