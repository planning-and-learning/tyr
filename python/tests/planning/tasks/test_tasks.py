# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
import pytest
from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext
from pytyr import planning
from pytyr.formalism import Parser
from python.tests.planning.support.components import GRIPPER


def test_planning_task_view_accessors_keep_temporary_owners_alive() -> None:
    parser_options = ParserOptions()
    parser = Parser(str(GRIPPER.domain_path), parser_options)
    formalism_task = parser.parse_task(
        str(GRIPPER.task_path),
        parser_options,
    )

    lifted_task_view = planning.lifted.Task(formalism_task).get_task()

    gc.collect()

    assert lifted_task_view.get_name() == "gripper-2"
    assert [object_.get_name() for object_ in lifted_task_view.get_objects()] == ["ball1", "ball2", "left", "right"]

    parser = Parser(str(GRIPPER.domain_path), parser_options)
    formalism_task = parser.parse_task(
        str(GRIPPER.task_path),
        parser_options,
    )
    ground_task_view = (
        planning.lifted.Task(formalism_task)
        .instantiate_ground_task(
            ExecutionContext(1),
            planning.lifted.GroundTaskInstantiationOptions(),
        )
        .task.get_task()
    )

    gc.collect()

    assert ground_task_view.get_name() == "gripper-2"
    assert [object_.get_name() for object_ in ground_task_view.get_objects()] == ["ball1", "ball2", "left", "right"]


def test_ground_task_instantiation_result_default_is_explicit_failure() -> None:
    result = planning.lifted.GroundTaskInstantiationResult()

    assert result.task is None
    assert result.status == planning.lifted.GroundTaskInstantiationStatus.PROVEN_UNSOLVABLE


@pytest.mark.parametrize(
    "options",
    [
        (),
        (planning.lifted.GroundTaskInstantiationOptions(),),
        (planning.lifted.GroundTaskInstantiationOptions(disable_invariant_synthesis=False),),
    ],
    ids=["omitted", "default", "invariant_synthesis"],
)
def test_lifted_task_instantiates_ground_task_from_parsed_pddl(
    options: tuple[planning.lifted.GroundTaskInstantiationOptions, ...],
) -> None:
    parser_options = ParserOptions()
    parser = Parser(str(GRIPPER.domain_path), parser_options)
    formalism_task = parser.parse_task(
        str(GRIPPER.task_path),
        parser_options,
    )

    lifted_task = planning.lifted.Task(formalism_task)
    result = lifted_task.instantiate_ground_task(ExecutionContext(1), *options)

    assert result.status == planning.lifted.GroundTaskInstantiationStatus.SUCCESS
    assert isinstance(result.task, planning.ground.Task)
    assert result.task.get_task().get_name() == "gripper-2"
