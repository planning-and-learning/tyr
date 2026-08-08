import gc
from typing import override

from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pytyr import datalog, planning
from pytyr.formalism import planning as formalism_planning
from pytyr.formalism.planning import Parser
from pyyggdrasil.execution import ExecutionContext

Node = planning.lifted.Node
State = planning.lifted.State
Plan = planning.lifted.Plan
IwStatistics = planning.lifted.iw.Statistics
SearchStatus = planning.SearchStatus

GRIPPER = fetch_task("classical/tests/gripper/test-1.pddl")


def _lifted_task() -> planning.lifted.Task:
    options = ParserOptions()
    parser = Parser(str(GRIPPER.domain_path), options)
    formalism_task = parser.parse_task(str(GRIPPER.task_path), options)
    return planning.lifted.Task(formalism_task)


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


def test_algorithm_event_handler_subclasses_can_call_super_constructor() -> None:
    class AStarEventHandler(planning.lifted.astar_eager.EventHandler):
        def __init__(self) -> None:
            super().__init__()
        @override
        def on_start_search(self, node: Node, f_value: float): pass
        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics): pass
        @override
        def on_solved(self, plan: Plan): pass

    class BRFSEventHandler(planning.lifted.brfs.EventHandler):
        def __init__(self) -> None:
            super().__init__()

        @override
        def on_start_search(self, node: Node): pass
        @override
        def on_finish_layer(self, layer: int, statistics: planning.Statistics): pass
        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics): pass
        @override
        def on_solved(self, plan: Plan): pass

    class GBFSEventHandler(planning.lifted.gbfs_lazy.EventHandler):
        def __init__(self) -> None:
            super().__init__()
        @override
        def on_start_search(self, node: Node, h_value: float): pass
        @override
        def on_new_best_h_value(self, h_value: float): pass
        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics): pass
        @override
        def on_solved(self, plan: Plan): pass

    class IWEventHandler(planning.lifted.iw.EventHandler):
        def __init__(self) -> None:
            super().__init__()
            self.algorithm_statistics = planning.lifted.iw.Statistics()

        @override
        def on_start_search(self, max_arity: int): pass
        @override
        def on_start_arity(self, arity: int): pass
        @override
        def on_end_arity(self, arity: int, status: SearchStatus): pass
        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics): pass
        @override
        def on_solved(self, arity: int): pass
        @override
        def get_statistics(self): return self.algorithm_statistics

    class SIWEventHandler(planning.lifted.siw.EventHandler):
        def __init__(self) -> None:
            super().__init__()
            self.algorithm_statistics = planning.lifted.siw.Statistics()

        @override
        def on_start_search(self): pass
        @override
        def on_start_subsearch(self, subsearch_index: int): pass
        @override
        def add_subsearch_statistics(self, search_statistics: planning.Statistics, solver_statistics: IwStatistics): pass
        @override
        def on_end_subsearch(self, subsearch_index: int, status: SearchStatus): pass
        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics): pass
        @override
        def on_solved(self, plan: Plan): pass
        @override
        def get_statistics(self): return self.algorithm_statistics

    for event_handler_class, event_handler_base in (
        (AStarEventHandler, planning.lifted.astar_eager.EventHandler),
        (BRFSEventHandler, planning.lifted.brfs.EventHandler),
        (GBFSEventHandler, planning.lifted.gbfs_lazy.EventHandler),
        (IWEventHandler, planning.lifted.iw.EventHandler),
        (SIWEventHandler, planning.lifted.siw.EventHandler),
    ):
        assert isinstance(event_handler_class(), event_handler_base)


def test_pruning_strategy_subclasses_can_call_super_constructor() -> None:
    class PythonPruningStrategy(planning.lifted.PruningStrategy):
        def __init__(self) -> None:
            super().__init__()

        @override
        def should_prune_state(self, state: State):
            return False

        @override
        def should_prune_successor_state(self, state: State, succ_state: State, is_new_succ_state: bool):
            return is_new_succ_state

    pruning_strategy = PythonPruningStrategy()

    assert isinstance(pruning_strategy, planning.lifted.PruningStrategy)


def test_goal_strategy_subclasses_can_call_super_constructor() -> None:
    task = _lifted_task()
    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.lifted.AxiomEvaluatorFactory().create(task, execution_context)
    state = planning.lifted.StateRepositoryFactory().create(task).get_initial_state(axiom_evaluator)

    class PythonGoalStrategy(planning.lifted.GoalStrategy):
        def __init__(self) -> None:
            super().__init__()
            self.saw_builder = False

        @override
        def is_static_goal_satisfied(self, task: planning.lifted.Task):
            return True

        @override
        def is_dynamic_goal_satisfied(self, seed_state: State, state: planning.lifted.StateBuilder):
            self.saw_builder = isinstance(state, planning.lifted.StateBuilder)
            return False

    goal_strategy = PythonGoalStrategy()

    assert isinstance(goal_strategy, planning.lifted.GoalStrategy)
    assert not planning.lifted.GoalStrategy.is_dynamic_goal_satisfied(goal_strategy, state, state)
    assert goal_strategy.saw_builder


def test_heuristic_subclasses_dispatch_virtual_methods_through_base_binding() -> None:
    task = _lifted_task()
    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.lifted.AxiomEvaluatorFactory().create(task, execution_context)
    state_repository = planning.lifted.StateRepositoryFactory().create(task)
    state = state_repository.get_initial_state(axiom_evaluator)
    goal = task.get_task().get_goal()

    class PythonHeuristic(planning.lifted.Heuristic):
        def __init__(self) -> None:
            super().__init__()
            self.goal: formalism_planning.GroundConjunctiveCondition | None = None
            self.saw_builder = False

        @override
        def set_goal(self, goal: formalism_planning.GroundConjunctiveCondition):
            self.goal = goal

        @override
        def evaluate(self, state: planning.lifted.StateBuilder):
            self.saw_builder = isinstance(state, planning.lifted.StateBuilder)
            return 7.0

    heuristic = PythonHeuristic()

    assert isinstance(heuristic, planning.lifted.Heuristic)

    planning.lifted.Heuristic.set_goal(heuristic, goal)
    value = planning.lifted.Heuristic.evaluate(heuristic, state)

    assert heuristic.goal == goal
    assert value == 7.0
    assert heuristic.saw_builder
