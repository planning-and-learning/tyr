# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from pyyggdrasil.execution import ExecutionContext
from typing_extensions import override
from pytyr import planning
from python.tests.planning.support.ground import StateBuilderView, ground_task


def test_pruning_strategy_subclasses_can_call_super_constructor() -> None:
    class PythonPruningStrategy(planning.ground.PruningStrategy):
        def __init__(self) -> None:
            super().__init__()

        @override
        def should_prune_state(self, state: StateBuilderView) -> bool:
            return False

        @override
        def should_prune_successor_state(
            self, state: StateBuilderView, succ_state: StateBuilderView, is_new_succ_state: bool
        ) -> bool:
            return is_new_succ_state

    pruning_strategy = PythonPruningStrategy()

    assert isinstance(pruning_strategy, planning.ground.PruningStrategy)


def test_goal_strategy_subclasses_can_call_super_constructor() -> None:
    task = ground_task()
    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.ground.AxiomEvaluator(task, execution_context)
    state = planning.ground.StateRepository(task).get_initial_state(axiom_evaluator)

    class PythonGoalStrategy(planning.ground.GoalStrategy):
        def __init__(self) -> None:
            super().__init__()
            self.saw_state = False

        @override
        def is_static_goal_satisfied(self, task: planning.ground.Task) -> bool:
            return True

        @override
        def is_dynamic_goal_satisfied(self, seed_state: StateBuilderView, state: StateBuilderView) -> bool:
            self.saw_state = isinstance(state, planning.ground.StateBuilderView)
            return False

    goal_strategy = PythonGoalStrategy()

    assert isinstance(goal_strategy, planning.ground.GoalStrategy)
    assert not planning.ground.GoalStrategy.is_dynamic_goal_satisfied(goal_strategy, state, state)
    assert goal_strategy.saw_state
