# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from python.tests.planning.support.lifted import StateBuilderView, lifted_task
from pyyggdrasil.execution import ExecutionContext
from typing_extensions import override

from pytyr import formalism, planning


def test_heuristic_subclasses_dispatch_virtual_methods_through_base_binding() -> None:
    task = lifted_task()
    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.lifted.AxiomEvaluator(task, execution_context)
    state_repository = planning.lifted.StateRepository(task)
    state = state_repository.get_initial_state(axiom_evaluator)
    goal = task.get_task().goal()

    class PythonHeuristic(planning.lifted.Heuristic):
        def __init__(self) -> None:
            super().__init__()
            self.goal: formalism.ConjunctiveConditionGround | None = None
            self.saw_state = False

        @override
        def set_goal(self, goal: formalism.ConjunctiveConditionGround) -> None:
            self.goal = goal

        @override
        def evaluate(self, state: StateBuilderView) -> float:
            self.saw_state = type(state) is planning.lifted.StateBuilderView
            return 7.0

    heuristic = PythonHeuristic()

    assert isinstance(heuristic, planning.lifted.Heuristic)

    planning.lifted.Heuristic.set_goal(heuristic, goal)
    value = planning.lifted.Heuristic.evaluate(heuristic, state)

    assert heuristic.goal == goal
    assert value == 7.0
    assert heuristic.saw_state
