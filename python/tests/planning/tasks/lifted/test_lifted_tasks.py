# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext
from pytyr import formalism, planning
from pytyr.formalism import Parser
from python.tests.planning.support.lifted import GRIPPER, Node


def test_a_lifted_task_leaves_its_formalism_task_usable() -> None:
    options = ParserOptions()
    parser = Parser(str(GRIPPER.domain_path), options)
    formalism_task = parser.parse_task(str(GRIPPER.task_path), options)
    text = repr(formalism_task)

    planning.lifted.Task(formalism_task)

    assert formalism_task.get_repository() is not None
    assert repr(formalism_task) == text


def test_lifted_tasks_of_one_formalism_task_write_into_their_own_layers() -> None:
    options = ParserOptions()
    formalism_task = Parser(str(GRIPPER.domain_path), options).parse_task(str(GRIPPER.task_path), options)
    frozen = len(formalism_task.get_fdr_context().get_variables())
    first = planning.lifted.Task(formalism_task)
    second = planning.lifted.Task(formalism_task)

    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.lifted.AxiomEvaluator(first, execution_context)
    state_repository = planning.lifted.StateRepository(first)
    successor_generator = planning.lifted.SuccessorGenerator(first, execution_context)
    initial = planning.lifted.get_initial_node(state_repository, axiom_evaluator)
    # The successor of each transition, generated and registered (a borrowed binding is valid only during its callback).
    successors: list[Node] = []

    def visit(transition: planning.lifted.TransitionLabel, successor_state: planning.lifted.SuccessorState) -> bool:
        planning.lifted.find_successor(state_repository, initial, successor_state)
        planning.lifted.extend_successor(axiom_evaluator, initial, successor_state)
        successors.append(planning.lifted.register_successor(state_repository, initial, successor_state))
        return True

    planning.lifted.deterministic.for_each_successor(successor_generator, initial, visit)
    assert successors

    assert len(first.get_fdr_context().get_variables()) > frozen
    assert len(second.get_fdr_context().get_variables()) == frozen
    assert len(formalism_task.get_fdr_context().get_variables()) == frozen
