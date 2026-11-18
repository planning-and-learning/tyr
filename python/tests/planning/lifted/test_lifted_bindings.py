# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
from pathlib import Path

from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pyyggdrasil.execution import ExecutionContext
from typing_extensions import override

from pytyr import datalog, planning
from pytyr.formalism import datalog as formalism_datalog, planning as formalism_planning
from pytyr.formalism.planning import Parser

Node = planning.lifted.Node
StateBuilderShared = planning.lifted.StateBuilderShared
StateBuilder = planning.lifted.StateBuilder
Plan = planning.lifted.Plan
IwStatistics = planning.algorithms.iw.lifted.Statistics
SearchStatus = planning.SearchStatus

GRIPPER = fetch_task("classical/tests/gripper/test-1.pddl")


def _lifted_task() -> planning.lifted.Task:
    options = ParserOptions()
    parser = Parser(str(GRIPPER.domain_path), options)
    formalism_task = parser.parse_task(str(GRIPPER.task_path), options)
    return planning.lifted.Task(formalism_task)


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
    initial = successor_generator.get_initial_node(state_repository, axiom_evaluator)
    assert successor_generator.get_labeled_successor_nodes(initial, state_repository, axiom_evaluator)

    assert len(first.get_fdr_context().get_variables()) > frozen
    assert len(second.get_fdr_context().get_variables()) == frozen
    assert len(formalism_task.get_fdr_context().get_variables()) == frozen


def test_lifted_datalog_workspaces_own_independent_repositories() -> None:
    task = _lifted_task()
    translated_program = planning.lifted.GroundTaskProgram(task.get_task())
    program = translated_program.get_datalog_program()
    first = datalog.lifted.UnannotatedProgramWorkspace(program)
    second = datalog.lifted.UnannotatedProgramWorkspace(program)

    assert first.get_workspace_repository() is not second.get_workspace_repository()

    first.reset_evaluation()

    del program, translated_program
    gc.collect()

    assert first.get_static_fact_sets() is not None
    assert second.get_static_fact_sets() is not None


def test_fact_sets_give_a_predicates_rows() -> None:
    fixture = Path(__file__).parents[4] / "tests/support/data/planning/algorithms"
    options = ParserOptions()
    task = planning.lifted.Task(
        Parser(str(fixture / "iw_goal_before_novelty_domain.pddl"), options).parse_task(
            str(fixture / "iw_goal_before_novelty_problem.pddl"), options
        )
    )
    program = planning.lifted.RPGProgram(task.get_task()).get_datalog_program()
    workspace = datalog.lifted.UnannotatedProgramWorkspace(program)
    facts = workspace.get_fluent_fact_sets()

    rows = list(facts.rows(0))
    assert all(isinstance(row, formalism_datalog.RowPredicateFluent) for row in rows)

    # The rows keep what they are read through alive.
    del facts, workspace, program, task
    gc.collect()
    assert all(row.get_relation().get_name() for row in rows)


def test_algorithm_event_handler_subclasses_can_call_super_constructor() -> None:
    class AStarEventHandler(planning.algorithms.astar_eager.lifted.EventHandler):
        def __init__(self) -> None:
            super().__init__()

        @override
        def on_start_search(self, node: Node, f_value: float) -> None:
            pass

        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics) -> None:
            pass

        @override
        def on_solved(self, plan: Plan) -> None:
            pass

    class BRFSEventHandler(planning.algorithms.brfs.lifted.EventHandler):
        def __init__(self) -> None:
            super().__init__()

        @override
        def on_start_search(self, node: Node) -> None:
            pass

        @override
        def on_finish_layer(self, layer: int, statistics: planning.Statistics) -> None:
            pass

        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics) -> None:
            pass

        @override
        def on_solved(self, plan: Plan) -> None:
            pass

    class GBFSEventHandler(planning.algorithms.gbfs_lazy.lifted.EventHandler):
        def __init__(self) -> None:
            super().__init__()

        @override
        def on_start_search(self, node: Node, h_value: float) -> None:
            pass

        @override
        def on_new_best_h_value(self, h_value: float) -> None:
            pass

        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics) -> None:
            pass

        @override
        def on_solved(self, plan: Plan) -> None:
            pass

    class IWEventHandler(planning.algorithms.iw.lifted.EventHandler):
        def __init__(self) -> None:
            super().__init__()
            self.algorithm_statistics = planning.algorithms.iw.lifted.Statistics()

        @override
        def on_start_search(self, max_arity: int) -> None:
            pass

        @override
        def on_start_arity(self, arity: int) -> None:
            pass

        @override
        def on_end_arity(self, arity: int, status: SearchStatus) -> None:
            pass

        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics) -> None:
            pass

        @override
        def on_solved(self, arity: int) -> None:
            pass

        @override
        def get_statistics(self) -> planning.algorithms.iw.lifted.Statistics:
            return self.algorithm_statistics

    class SIWEventHandler(planning.algorithms.siw.lifted.EventHandler):
        def __init__(self) -> None:
            super().__init__()
            self.algorithm_statistics = planning.algorithms.siw.lifted.Statistics()

        @override
        def on_start_search(self) -> None:
            pass

        @override
        def on_start_subsearch(self, subsearch_index: int) -> None:
            pass

        @override
        def add_subsearch_statistics(
            self, search_statistics: planning.Statistics, solver_statistics: IwStatistics
        ) -> None:
            pass

        @override
        def on_end_subsearch(self, subsearch_index: int, status: SearchStatus) -> None:
            pass

        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics) -> None:
            pass

        @override
        def on_solved(self, plan: Plan) -> None:
            pass

        @override
        def get_statistics(self) -> planning.algorithms.siw.lifted.Statistics:
            return self.algorithm_statistics

    for event_handler_class, event_handler_base in (
        (AStarEventHandler, planning.algorithms.astar_eager.lifted.EventHandler),
        (BRFSEventHandler, planning.algorithms.brfs.lifted.EventHandler),
        (GBFSEventHandler, planning.algorithms.gbfs_lazy.lifted.EventHandler),
        (IWEventHandler, planning.algorithms.iw.lifted.EventHandler),
        (SIWEventHandler, planning.algorithms.siw.lifted.EventHandler),
    ):
        assert isinstance(event_handler_class(), event_handler_base)


def test_pruning_strategy_subclasses_can_call_super_constructor() -> None:
    class PythonPruningStrategy(planning.lifted.PruningStrategy):
        def __init__(self) -> None:
            super().__init__()

        @override
        def should_prune_state(self, state: StateBuilderShared) -> bool:
            return False

        @override
        def should_prune_successor_state(self, state: StateBuilderShared, succ_state: StateBuilderShared, is_new_succ_state: bool) -> bool:
            return is_new_succ_state

    pruning_strategy = PythonPruningStrategy()

    assert isinstance(pruning_strategy, planning.lifted.PruningStrategy)


def test_goal_strategy_subclasses_can_call_super_constructor() -> None:
    task = _lifted_task()
    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.lifted.AxiomEvaluator(task, execution_context)
    state = planning.lifted.StateRepository(task).get_initial_state(axiom_evaluator)

    class PythonGoalStrategy(planning.lifted.GoalStrategy):
        def __init__(self) -> None:
            super().__init__()
            self.saw_state = False

        @override
        def is_static_goal_satisfied(self, task: planning.lifted.Task) -> bool:
            return True

        @override
        def is_dynamic_goal_satisfied(self, seed_state: StateBuilderShared, state: StateBuilderShared | StateBuilder) -> bool:
            self.saw_state = isinstance(state, planning.lifted.StateBuilderShared)
            return False

    goal_strategy = PythonGoalStrategy()

    assert isinstance(goal_strategy, planning.lifted.GoalStrategy)
    assert not planning.lifted.GoalStrategy.is_dynamic_goal_satisfied(goal_strategy, state, state)
    assert goal_strategy.saw_state


def test_heuristic_subclasses_dispatch_virtual_methods_through_base_binding() -> None:
    task = _lifted_task()
    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.lifted.AxiomEvaluator(task, execution_context)
    state_repository = planning.lifted.StateRepository(task)
    state = state_repository.get_initial_state(axiom_evaluator)
    goal = task.get_task().get_goal()

    class PythonHeuristic(planning.lifted.Heuristic):
        def __init__(self) -> None:
            super().__init__()
            self.goal: formalism_planning.ConjunctiveConditionGround | None = None
            self.saw_state = False

        @override
        def set_goal(self, goal: formalism_planning.ConjunctiveConditionGround) -> None:
            self.goal = goal

        @override
        def evaluate(self, state: StateBuilderShared) -> float:
            self.saw_state = type(state) is planning.lifted.StateBuilderShared
            return 7.0

    heuristic = PythonHeuristic()

    assert isinstance(heuristic, planning.lifted.Heuristic)

    planning.lifted.Heuristic.set_goal(heuristic, goal)
    value = planning.lifted.Heuristic.evaluate(heuristic, state)

    assert heuristic.goal == goal
    assert value == 7.0
    assert heuristic.saw_state
