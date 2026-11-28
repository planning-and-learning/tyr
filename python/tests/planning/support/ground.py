# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pyyggdrasil.execution import ExecutionContext

from pytyr import planning
from pytyr.formalism import Parser

StateBuilderView = planning.ground.StateBuilderView


GRIPPER = fetch_task("classical/tests/gripper/test-1.pddl")


def ground_task() -> planning.ground.Task:
    options = ParserOptions()
    parser = Parser(str(GRIPPER.domain_path), options)
    formalism_task = parser.parse_task(str(GRIPPER.task_path), options)
    lifted_task = planning.lifted.Task(formalism_task)
    result = lifted_task.instantiate_ground_task(
        ExecutionContext(1),
        planning.lifted.GroundTaskInstantiationOptions(),
    )
    return result.task
