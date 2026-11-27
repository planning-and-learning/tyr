# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from pyyggdrasil.execution import ExecutionContext
from typing_extensions import override
from pytyr import formalism, planning
from python.tests.planning.support.ground import StateBuilderView, ground_task


def test_heuristic_subclasses_dispatch_virtual_methods_through_base_binding() -> None:
    task = ground_task()
    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.ground.AxiomEvaluator(task, execution_context)
    state_repository = planning.ground.StateRepository(task)
    state = state_repository.get_initial_state(axiom_evaluator)
    goal = task.get_task().get_goal()

    class PythonHeuristic(planning.ground.Heuristic):
        def __init__(self) -> None:
            super().__init__()
            self.goal: formalism.ConjunctiveConditionGround | None = None
            self.saw_state = False

        @override
        def set_goal(self, goal: formalism.ConjunctiveConditionGround) -> None:
            self.goal = goal

        @override
        def evaluate(self, state: StateBuilderView) -> float:
            self.saw_state = type(state) is planning.ground.StateBuilderView
            return 7.0

    heuristic = PythonHeuristic()

    assert isinstance(heuristic, planning.ground.Heuristic)

    planning.ground.Heuristic.set_goal(heuristic, goal)
    value = planning.ground.Heuristic.evaluate(heuristic, state)

    assert heuristic.goal == goal
    assert value == 7.0
    assert heuristic.saw_state
