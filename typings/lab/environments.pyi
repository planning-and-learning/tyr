# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import subprocess as subprocess

from lab.steps import Step

class Environment:
    def __init__(self, randomize_task_order: bool = True) -> None: ...

class LocalEnvironment(Environment):
    def __init__(self, processes: int | None = None, randomize_task_order: bool = True) -> None: ...
    def run_steps(self, steps: list[Step]) -> None: ...

class SlurmEnvironment(Environment):
    DEFAULT_EXPORT: list[str]
    DEFAULT_SETUP: str
    MAX_TASKS: int
    export: list[str]
    def __init__(
        self,
        email: str | None = None,
        extra_options: str | None = None,
        partition: str | None = None,
        qos: str | None = None,
        time_limit_per_task: str | None = None,
        memory_per_cpu: str | None = None,
        cpus_per_task: int = 1,
        export: list[str] | None = None,
        setup: str | None = None,
        randomize_task_order: bool = True,
    ) -> None: ...
    def run_steps(self, steps: list[Step]) -> None: ...

class ArrheniusEnvironment(SlurmEnvironment): ...
