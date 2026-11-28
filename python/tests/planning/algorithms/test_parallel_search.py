# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
from datetime import timedelta

import pytest
from python.tests.planning.support.components import (
    GroundNode,
    LiftedNode,
    algorithm_bindings,
    make_components_ground,
    make_components_lifted,
    make_gripper_tasks,
)
from pyyggdrasil.execution import ExecutionContext
from typing_extensions import override

from pytyr import formalism, planning

SearchResult = planning.lifted.SearchResult | planning.ground.SearchResult


def _assert_worker_statistics(result: SearchResult, num_workers: int) -> None:
    assert len(result.worker_statistics) == num_workers
    for getter in (
        "get_num_generated_successors",
        "get_num_expanded",
        "get_num_deadends",
        "get_num_pruned",
        "get_num_generated_candidates",
        "get_num_transferred_candidates",
        "get_num_registered_states",
        "get_state_storage_memory_usage",
    ):
        worker_total = sum(getattr(worker, getter)() for worker in result.worker_statistics)
        assert worker_total == getattr(result.statistics, getter)()
    assert result.statistics.get_communication_overhead() == (
        result.statistics.get_num_transferred_candidates() / result.statistics.get_num_generated_candidates()
        if result.statistics.get_num_generated_candidates()
        else 0
    )
    worker_idle_time = sum(
        (worker.get_idle_time() for worker in result.worker_statistics),
        start=timedelta(0),
    )
    assert abs(worker_idle_time - result.statistics.get_idle_time()) <= timedelta(microseconds=num_workers)
    assert sum(worker.get_num_destination_lock_acquisitions() for worker in result.worker_statistics) == (
        result.statistics.get_num_destination_lock_acquisitions()
    )
    worker_lock_wait_time = sum(
        (worker.get_destination_lock_wait_time() for worker in result.worker_statistics),
        start=timedelta(0),
    )
    assert abs(worker_lock_wait_time - result.statistics.get_destination_lock_wait_time()) <= timedelta(
        microseconds=num_workers
    )
    worker_lock_hold_time = sum(
        (worker.get_destination_lock_hold_time() for worker in result.worker_statistics),
        start=timedelta(0),
    )
    assert abs(worker_lock_hold_time - result.statistics.get_destination_lock_hold_time()) <= timedelta(
        microseconds=num_workers
    )


def _check_parallel_search_ground(task: planning.ground.Task) -> None:
    class EventHandler(planning.algorithms.gbfs_lazy.ground.EventHandler):
        def __init__(self) -> None:
            super().__init__()
            self.end_status: planning.SearchStatus | None = None
            self.num_expanded: int | None = None
            self.solved = False

        @override
        def on_start_search(self, node: GroundNode, h_value: float) -> None:
            pass

        @override
        def on_new_best_h_value(self, h_value: float) -> None:
            pass

        @override
        def on_end_search(self, status: planning.SearchStatus, statistics: planning.Statistics) -> None:
            self.end_status = status
            self.num_expanded = statistics.get_num_expanded()

        @override
        def on_solved(self, plan: planning.ground.deterministic.PlanBuilderView) -> None:
            self.solved = True

    state_repository, axiom_evaluator, successor_generator = make_components_ground(task, concurrent=True)
    event_handler = EventHandler()
    options = planning.algorithms.gbfs_lazy.ground.Options()
    options.event_handler = event_handler
    options.num_search_workers = 2
    options.collect_destination_lock_statistics = True

    result = planning.algorithms.gbfs_lazy.ground.find_solution(
        task,
        state_repository,
        axiom_evaluator,
        successor_generator,
        planning.ground.BlindHeuristic(),
        options,
    )

    assert result.status == planning.SearchStatus.SOLVED
    assert result.plan is not None
    assert result.goal_node is not None
    assert result.goal_node.state().context() == state_repository
    assert result.statistics.get_num_expanded() > 0
    assert result.statistics.get_num_destination_lock_acquisitions() > 0
    assert event_handler.end_status == result.status
    assert event_handler.num_expanded == result.statistics.get_num_expanded()
    assert event_handler.solved
    _assert_worker_statistics(result, 2)

    brfs_options = planning.algorithms.brfs.ground.Options()
    brfs_options.num_search_workers = 2
    brfs_result = planning.algorithms.brfs.ground.find_solution(
        task,
        state_repository,
        axiom_evaluator,
        successor_generator,
        brfs_options,
    )
    assert brfs_result.status == planning.SearchStatus.SOLVED
    assert brfs_result.goal_node is not None
    assert brfs_result.goal_node.state().context() == state_repository
    _assert_worker_statistics(brfs_result, 2)

    astar_options = planning.algorithms.astar_eager.ground.Options()
    astar_options.num_search_workers = 2
    for mode in (planning.ParallelSearchMode.SYNCHRONOUS, planning.ParallelSearchMode.ASYNCHRONOUS):
        astar_options.parallel_search_mode = mode
        astar_result = planning.algorithms.astar_eager.ground.find_solution(
            task,
            state_repository,
            axiom_evaluator,
            successor_generator,
            planning.ground.BlindHeuristic(),
            astar_options,
        )
        assert astar_result.status == planning.SearchStatus.SOLVED
        _assert_worker_statistics(astar_result, 2)


def _check_parallel_search_lifted(task: planning.lifted.Task) -> None:
    class EventHandler(planning.algorithms.gbfs_lazy.lifted.EventHandler):
        def __init__(self) -> None:
            super().__init__()
            self.end_status: planning.SearchStatus | None = None
            self.num_expanded: int | None = None
            self.solved = False

        @override
        def on_start_search(self, node: LiftedNode, h_value: float) -> None:
            pass

        @override
        def on_new_best_h_value(self, h_value: float) -> None:
            pass

        @override
        def on_end_search(self, status: planning.SearchStatus, statistics: planning.Statistics) -> None:
            self.end_status = status
            self.num_expanded = statistics.get_num_expanded()

        @override
        def on_solved(self, plan: planning.lifted.deterministic.PlanBuilderView) -> None:
            self.solved = True

    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task, concurrent=True)
    event_handler = EventHandler()
    options = planning.algorithms.gbfs_lazy.lifted.Options()
    options.event_handler = event_handler
    options.num_search_workers = 2
    options.collect_destination_lock_statistics = True

    result = planning.algorithms.gbfs_lazy.lifted.find_solution(
        task,
        state_repository,
        axiom_evaluator,
        successor_generator,
        planning.lifted.BlindHeuristic(),
        options,
    )

    assert result.status == planning.SearchStatus.SOLVED
    assert result.plan is not None
    assert result.goal_node is not None
    assert result.goal_node.state().context() == state_repository
    assert result.statistics.get_num_expanded() > 0
    assert result.statistics.get_num_destination_lock_acquisitions() > 0
    assert event_handler.end_status == result.status
    assert event_handler.num_expanded == result.statistics.get_num_expanded()
    assert event_handler.solved
    _assert_worker_statistics(result, 2)

    brfs_options = planning.algorithms.brfs.lifted.Options()
    brfs_options.num_search_workers = 2
    brfs_result = planning.algorithms.brfs.lifted.find_solution(
        task,
        state_repository,
        axiom_evaluator,
        successor_generator,
        brfs_options,
    )
    assert brfs_result.status == planning.SearchStatus.SOLVED
    assert brfs_result.goal_node is not None
    assert brfs_result.goal_node.state().context() == state_repository
    _assert_worker_statistics(brfs_result, 2)

    astar_options = planning.algorithms.astar_eager.lifted.Options()
    astar_options.num_search_workers = 2
    for mode in (planning.ParallelSearchMode.SYNCHRONOUS, planning.ParallelSearchMode.ASYNCHRONOUS):
        astar_options.parallel_search_mode = mode
        astar_result = planning.algorithms.astar_eager.lifted.find_solution(
            task,
            state_repository,
            axiom_evaluator,
            successor_generator,
            planning.lifted.BlindHeuristic(),
            astar_options,
        )
        assert astar_result.status == planning.SearchStatus.SOLVED
        _assert_worker_statistics(astar_result, 2)


def _check_null_python_heuristic_workers_ground(task: planning.ground.Task) -> None:
    class NullWorkerHeuristic(planning.ground.Heuristic):
        def __init__(self) -> None:
            super().__init__()

        @override
        def set_goal(self, goal: formalism.ConjunctiveConditionGround) -> None:
            pass

        @override
        def evaluate(self, state: planning.ground.StateBuilderView) -> float:
            return 0.0

        def make_worker(self, _execution_context: ExecutionContext) -> None:
            return None

    state_repository, axiom_evaluator, successor_generator = make_components_ground(task)
    options = planning.algorithms.astar_eager.ground.Options()
    options.num_search_workers = 2

    with pytest.raises(ValueError, match="heuristic does not support worker construction"):
        planning.algorithms.astar_eager.ground.find_solution(
            task,
            state_repository,
            axiom_evaluator,
            successor_generator,
            NullWorkerHeuristic(),
            options,
        )


def _check_null_python_heuristic_workers_lifted(task: planning.lifted.Task) -> None:
    class NullWorkerHeuristic(planning.lifted.Heuristic):
        def __init__(self) -> None:
            super().__init__()

        @override
        def set_goal(self, goal: formalism.ConjunctiveConditionGround) -> None:
            pass

        @override
        def evaluate(self, state: planning.lifted.StateBuilderView) -> float:
            return 0.0

        def make_worker(self, _execution_context: ExecutionContext) -> None:
            return None

    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task)
    options = planning.algorithms.astar_eager.lifted.Options()
    options.num_search_workers = 2

    with pytest.raises(ValueError, match="heuristic does not support worker construction"):
        planning.algorithms.astar_eager.lifted.find_solution(
            task,
            state_repository,
            axiom_evaluator,
            successor_generator,
            NullWorkerHeuristic(),
            options,
        )


def _check_python_goal_and_pruning_workers_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_ground(task, concurrent=True)
    initial_state = state_repository.get_initial_state(axiom_evaluator)
    fluent_variables = [fact.variable() for fact in initial_state.fluent_facts()]
    factory_indices: dict[str, list[tuple[type[object], int]]] = {"goal": [], "pruning": []}
    worker_ids: dict[str, set[int]] = {"goal": set(), "pruning": set()}
    callback_indices: dict[str, list[int | None]] = {"goal": [], "pruning": []}
    # A state a callback keeps outlives the call only as its own copy (the search reuses the builder it is handed).
    retained_snapshots: list[tuple[planning.ground.StateBuilder, tuple[formalism.FDRValue, ...]]] = []

    class PythonGoalStrategy(planning.ground.GoalStrategy):
        def __init__(self, worker_index: int | None = None) -> None:
            super().__init__()
            self.worker_index = worker_index

        @override
        def make_worker(self, worker_index: planning.WorkerIndex) -> "PythonGoalStrategy":
            factory_indices["goal"].append((type(worker_index), int(worker_index)))
            worker = PythonGoalStrategy(int(worker_index))
            worker_ids["goal"].add(id(worker))
            return worker

        @override
        def is_static_goal_satisfied(self, task: planning.ground.Task) -> bool:
            callback_indices["goal"].append(self.worker_index)
            return True

        @override
        def is_dynamic_goal_satisfied(
            self,
            seed_state: planning.ground.StateBuilderView,
            state: planning.ground.StateBuilderView,
        ) -> bool:
            callback_indices["goal"].append(self.worker_index)
            if len(callback_indices["goal"]) > 2 and not retained_snapshots:
                kept = state_repository.get_state_builder()
                kept.assign(state)
                retained_snapshots.append((kept, tuple(state.get(variable) for variable in fluent_variables)))
            return False

    class PythonPruningStrategy(planning.ground.PruningStrategy):
        def __init__(self, worker_index: int | None = None) -> None:
            super().__init__()
            self.worker_index = worker_index

        @override
        def make_worker(self, worker_index: planning.WorkerIndex) -> "PythonPruningStrategy":
            factory_indices["pruning"].append((type(worker_index), int(worker_index)))
            worker = PythonPruningStrategy(int(worker_index))
            worker_ids["pruning"].add(id(worker))
            return worker

        @override
        def should_prune_state(self, state: planning.ground.StateBuilderView) -> bool:
            callback_indices["pruning"].append(self.worker_index)
            return False

        @override
        def should_prune_successor_state(
            self,
            state: planning.ground.StateBuilderView,
            succ_state: planning.ground.StateBuilderView,
            is_new_succ_state: bool,
        ) -> bool:
            callback_indices["pruning"].append(self.worker_index)
            return False

    options = planning.algorithms.brfs.ground.Options()
    options.num_search_workers = 2
    options.goal_strategy = PythonGoalStrategy()
    options.pruning_strategy = PythonPruningStrategy()

    result = planning.algorithms.brfs.ground.find_solution(
        task, state_repository, axiom_evaluator, successor_generator, options
    )

    assert result.status == planning.SearchStatus.EXHAUSTED
    for strategy in ("goal", "pruning"):
        assert factory_indices[strategy] == [(planning.WorkerIndex, 1)]
        assert len(worker_ids[strategy]) == 1
        assert set(callback_indices[strategy]) == {None, 1}

    assert retained_snapshots
    snapshot, values = retained_snapshots[0]
    gc.collect()
    assert tuple(snapshot.view(state_repository).get(variable) for variable in fluent_variables) == values


def _check_python_goal_and_pruning_workers_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task, concurrent=True)
    initial_state = state_repository.get_initial_state(axiom_evaluator)
    fluent_variables = [fact.variable() for fact in initial_state.fluent_facts()]
    factory_indices: dict[str, list[tuple[type[object], int]]] = {"goal": [], "pruning": []}
    worker_ids: dict[str, set[int]] = {"goal": set(), "pruning": set()}
    callback_indices: dict[str, list[int | None]] = {"goal": [], "pruning": []}
    # A state a callback keeps outlives the call only as its own copy (the search reuses the builder it is handed).
    retained_snapshots: list[tuple[planning.lifted.StateBuilder, tuple[formalism.FDRValue, ...]]] = []

    class PythonGoalStrategy(planning.lifted.GoalStrategy):
        def __init__(self, worker_index: int | None = None) -> None:
            super().__init__()
            self.worker_index = worker_index

        @override
        def make_worker(self, worker_index: planning.WorkerIndex) -> "PythonGoalStrategy":
            factory_indices["goal"].append((type(worker_index), int(worker_index)))
            worker = PythonGoalStrategy(int(worker_index))
            worker_ids["goal"].add(id(worker))
            return worker

        @override
        def is_static_goal_satisfied(self, task: planning.lifted.Task) -> bool:
            callback_indices["goal"].append(self.worker_index)
            return True

        @override
        def is_dynamic_goal_satisfied(
            self,
            seed_state: planning.lifted.StateBuilderView,
            state: planning.lifted.StateBuilderView,
        ) -> bool:
            callback_indices["goal"].append(self.worker_index)
            if len(callback_indices["goal"]) > 2 and not retained_snapshots:
                kept = state_repository.get_state_builder()
                kept.assign(state)
                retained_snapshots.append((kept, tuple(state.get(variable) for variable in fluent_variables)))
            return False

    class PythonPruningStrategy(planning.lifted.PruningStrategy):
        def __init__(self, worker_index: int | None = None) -> None:
            super().__init__()
            self.worker_index = worker_index

        @override
        def make_worker(self, worker_index: planning.WorkerIndex) -> "PythonPruningStrategy":
            factory_indices["pruning"].append((type(worker_index), int(worker_index)))
            worker = PythonPruningStrategy(int(worker_index))
            worker_ids["pruning"].add(id(worker))
            return worker

        @override
        def should_prune_state(self, state: planning.lifted.StateBuilderView) -> bool:
            callback_indices["pruning"].append(self.worker_index)
            return False

        @override
        def should_prune_successor_state(
            self,
            state: planning.lifted.StateBuilderView,
            succ_state: planning.lifted.StateBuilderView,
            is_new_succ_state: bool,
        ) -> bool:
            callback_indices["pruning"].append(self.worker_index)
            return False

    options = planning.algorithms.brfs.lifted.Options()
    options.num_search_workers = 2
    options.goal_strategy = PythonGoalStrategy()
    options.pruning_strategy = PythonPruningStrategy()

    result = planning.algorithms.brfs.lifted.find_solution(
        task, state_repository, axiom_evaluator, successor_generator, options
    )

    assert result.status == planning.SearchStatus.EXHAUSTED
    for strategy in ("goal", "pruning"):
        assert factory_indices[strategy] == [(planning.WorkerIndex, 1)]
        assert len(worker_ids[strategy]) == 1
        assert set(callback_indices[strategy]) == {None, 1}

    assert retained_snapshots
    snapshot, values = retained_snapshots[0]
    gc.collect()
    assert tuple(snapshot.view(state_repository).get(variable) for variable in fluent_variables) == values


def _check_missing_or_null_python_strategy_workers_ground(task: planning.ground.Task) -> None:
    class MissingWorkerGoalStrategy(planning.ground.GoalStrategy):
        @override
        def is_static_goal_satisfied(self, task: planning.ground.Task) -> bool:
            return True

        @override
        def is_dynamic_goal_satisfied(
            self,
            seed_state: planning.ground.StateBuilderView,
            state: planning.ground.StateBuilderView,
        ) -> bool:
            return False

    class MissingWorkerPruningStrategy(planning.ground.PruningStrategy):
        pass

    state_repository, axiom_evaluator, successor_generator = make_components_ground(task)
    cases = (
        ("goal_strategy", MissingWorkerGoalStrategy(), "goal strategy does not support worker construction"),
        (
            "pruning_strategy",
            MissingWorkerPruningStrategy(),
            "pruning strategy does not support worker construction",
        ),
    )

    for field, strategy, error in cases:
        options = planning.algorithms.brfs.ground.Options()
        options.num_search_workers = 2
        setattr(options, field, strategy)
        with pytest.raises(ValueError, match=error):
            planning.algorithms.brfs.ground.find_solution(
                task, state_repository, axiom_evaluator, successor_generator, options
            )


def _check_missing_or_null_python_strategy_workers_lifted(task: planning.lifted.Task) -> None:
    class MissingWorkerGoalStrategy(planning.lifted.GoalStrategy):
        @override
        def is_static_goal_satisfied(self, task: planning.lifted.Task) -> bool:
            return True

        @override
        def is_dynamic_goal_satisfied(
            self,
            seed_state: planning.lifted.StateBuilderView,
            state: planning.lifted.StateBuilderView,
        ) -> bool:
            return False

    class MissingWorkerPruningStrategy(planning.lifted.PruningStrategy):
        pass

    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task)
    cases = (
        ("goal_strategy", MissingWorkerGoalStrategy(), "goal strategy does not support worker construction"),
        (
            "pruning_strategy",
            MissingWorkerPruningStrategy(),
            "pruning strategy does not support worker construction",
        ),
    )

    for field, strategy, error in cases:
        options = planning.algorithms.brfs.lifted.Options()
        options.num_search_workers = 2
        setattr(options, field, strategy)
        with pytest.raises(ValueError, match=error):
            planning.algorithms.brfs.lifted.find_solution(
                task, state_repository, axiom_evaluator, successor_generator, options
            )


def test_parallel_search_is_available_through_python_bindings() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_parallel_search_ground(ground_task)
    _check_parallel_search_lifted(lifted_task)


def test_parallel_search_rejects_null_python_heuristic_workers() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_null_python_heuristic_workers_ground(ground_task)
    _check_null_python_heuristic_workers_lifted(lifted_task)


def test_parallel_search_constructs_python_goal_and_pruning_workers() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_python_goal_and_pruning_workers_ground(ground_task)
    _check_python_goal_and_pruning_workers_lifted(lifted_task)


def test_parallel_search_rejects_missing_or_null_python_strategy_workers() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_missing_or_null_python_strategy_workers_ground(ground_task)
    _check_missing_or_null_python_strategy_workers_lifted(lifted_task)


def test_parallel_astar_search_mode_is_bound() -> None:
    assert planning.ParallelSearchMode.SYNCHRONOUS != planning.ParallelSearchMode.ASYNCHRONOUS

    for task_module in (planning.ground, planning.lifted):
        options = algorithm_bindings(task_module, "astar_eager").Options()
        assert options.parallel_search_mode == planning.ParallelSearchMode.SYNCHRONOUS
        options.parallel_search_mode = planning.ParallelSearchMode.ASYNCHRONOUS
        assert options.parallel_search_mode == planning.ParallelSearchMode.ASYNCHRONOUS


def test_dist_hash_mode_is_bound() -> None:
    assert planning.DistHashMode.RANDOM != planning.DistHashMode.LMCUT

    for task_module in (planning.ground, planning.lifted):
        for algorithm_module in (
            algorithm_bindings(task_module, "astar_eager"),
            algorithm_bindings(task_module, "brfs"),
            algorithm_bindings(task_module, "gbfs_lazy"),
        ):
            options = algorithm_module.Options()
            assert options.dist_hash_mode == planning.DistHashMode.RANDOM
            options.dist_hash_mode = planning.DistHashMode.LMCUT
            assert options.dist_hash_mode == planning.DistHashMode.LMCUT
