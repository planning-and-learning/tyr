# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task

from pytyr import planning
from pytyr.formalism import Parser

Node = planning.lifted.OwnedNodeBuilderView | planning.lifted.NodeBuilderView


StateBuilderView = planning.lifted.StateBuilderView


GRIPPER = fetch_task("classical/tests/gripper/test-1.pddl")


def lifted_task() -> planning.lifted.Task:
    options = ParserOptions()
    parser = Parser(str(GRIPPER.domain_path), options)
    formalism_task = parser.parse_task(str(GRIPPER.task_path), options)
    return planning.lifted.Task(formalism_task)
