from typing import override

from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pytyr import planning
from pytyr.formalism import planning as formalism_planning
from pytyr.formalism.planning import Parser
from pyyggdrasil.execution import ExecutionContext

Node = planning.ground.Node
State = planning.ground.State
Plan = planning.ground.Plan
IwStatistics = planning.ground.iw.Statistics
SearchStatus = planning.SearchStatus

GRIPPER = fetch_task("classical/tests/gripper/test-1.pddl")


def _ground_task() -> planning.ground.Task:
    options = ParserOptions()
    parser = Parser(str(GRIPPER.domain_path), options)
    formalism_task = parser.parse_task(str(GRIPPER.task_path), options)
    lifted_task = planning.lifted.Task(formalism_task)
    result = lifted_task.instantiate_ground_task(
        ExecutionContext(1),
        planning.lifted.GroundTaskInstantiationOptions(),
    )
    return result.task


def test_algorithm_event_handler_subclasses_can_call_super_constructor() -> None:
    class AStarEventHandler(planning.ground.astar_eager.EventHandler):
        def __init__(self) -> None:
            super().__init__()
        @override
        def on_start_search(self, node: Node, f_value: float): pass
        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics): pass
        @override
        def on_solved(self, plan: Plan): pass

    class BRFSEventHandler(planning.ground.brfs.EventHandler):
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

    class GBFSEventHandler(planning.ground.gbfs_lazy.EventHandler):
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

    class IWEventHandler(planning.ground.iw.EventHandler):
        def __init__(self) -> None:
            super().__init__()
            self.algorithm_statistics = planning.ground.iw.Statistics()

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

    class SIWEventHandler(planning.ground.siw.EventHandler):
        def __init__(self) -> None:
            super().__init__()
            self.algorithm_statistics = planning.ground.siw.Statistics()

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
        (AStarEventHandler, planning.ground.astar_eager.EventHandler),
        (BRFSEventHandler, planning.ground.brfs.EventHandler),
        (GBFSEventHandler, planning.ground.gbfs_lazy.EventHandler),
        (IWEventHandler, planning.ground.iw.EventHandler),
        (SIWEventHandler, planning.ground.siw.EventHandler),
    ):
        assert isinstance(event_handler_class(), event_handler_base)


def test_pruning_strategy_subclasses_can_call_super_constructor() -> None:
    class PythonPruningStrategy(planning.ground.PruningStrategy):
        def __init__(self) -> None:
            super().__init__()

        @override
        def should_prune_state(self, state: State):
            return False

        @override
        def should_prune_successor_state(self, state: State, succ_state: State, is_new_succ_state: bool):
            return is_new_succ_state

    pruning_strategy = PythonPruningStrategy()

    assert isinstance(pruning_strategy, planning.ground.PruningStrategy)


def test_goal_strategy_subclasses_can_call_super_constructor() -> None:
    class PythonGoalStrategy(planning.ground.GoalStrategy):
        def __init__(self) -> None:
            super().__init__()

        @override
        def is_static_goal_satisfied(self, task: planning.ground.Task):
            return True

        @override
        def is_dynamic_goal_satisfied(self, seed_state: State, state: State):
            return False

    goal_strategy = PythonGoalStrategy()

    assert isinstance(goal_strategy, planning.ground.GoalStrategy)


def test_heuristic_subclasses_dispatch_virtual_methods_through_base_binding() -> None:
    task = _ground_task()
    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.ground.AxiomEvaluatorFactory().create(task, execution_context)
    state_repository = planning.ground.StateRepositoryFactory().create(task, axiom_evaluator)
    state = state_repository.get_initial_state()
    goal = task.get_task().get_goal()

    class PythonHeuristic(planning.ground.Heuristic):
        def __init__(self) -> None:
            super().__init__()
            self.goal: formalism_planning.GroundConjunctiveCondition | None = None
            self.evaluated_states: list[State] = []

        @override
        def set_goal(self, goal: formalism_planning.GroundConjunctiveCondition):
            self.goal = goal

        @override
        def evaluate(self, state: State):
            self.evaluated_states.append(state)
            return 7.0

    heuristic = PythonHeuristic()

    assert isinstance(heuristic, planning.ground.Heuristic)

    planning.ground.Heuristic.set_goal(heuristic, goal)
    value = planning.ground.Heuristic.evaluate(heuristic, state)

    assert heuristic.goal == goal
    assert value == 7.0
    assert heuristic.evaluated_states == [state]
