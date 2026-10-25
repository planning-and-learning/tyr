# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
from collections import Counter
from collections.abc import Callable, Sequence
from datetime import timedelta
from typing import Never, Self, TypeVar

import pytest
from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pyyggdrasil.execution import ExecutionContext
from typing_extensions import override

from pytyr import planning
from pytyr.formalism import planning as formalism_planning
from pytyr.formalism.planning import Parser

SearchResult = planning.lifted.SearchResult | planning.ground.SearchResult

GRIPPER = fetch_task("classical/tests/gripper/test-1.pddl")


def test_planning_modules_export_expected_algorithm_submodules() -> None:
    assert hasattr(planning, "ground")
    assert hasattr(planning, "lifted")

    for task_module in (planning.ground, planning.lifted):
        for algorithm_name in ("astar_eager", "brfs", "gbfs_lazy", "iw", "siw"):
            algorithm_module = getattr(task_module, algorithm_name)
            assert hasattr(algorithm_module, "Options")
            assert hasattr(algorithm_module, "EventHandler")
            assert hasattr(algorithm_module, "DefaultEventHandler")
            assert hasattr(algorithm_module, "find_solution")

        for algorithm_name in ("astar_eager", "brfs", "gbfs_lazy", "iw", "siw"):
            solver = getattr(task_module, algorithm_name).Solver()
            assert solver.options is not None
            if algorithm_name in ("astar_eager", "brfs", "gbfs_lazy"):
                assert solver.task is None
                assert solver.state_repository is None
                assert solver.axiom_evaluator is None
                assert solver.successor_generator is None
            if algorithm_name in ("astar_eager", "gbfs_lazy"):
                assert solver.heuristic is None
            with pytest.raises(ValueError, match="task is required"):
                solver.solve()


def test_planning_statistics_bindings_expose_counters_and_progress_snapshots() -> None:
    statistics = planning.Statistics()

    assert statistics.get_num_generated_successors() == 0
    assert statistics.get_num_expanded() == 0
    assert statistics.get_num_deadends() == 0
    assert statistics.get_num_pruned() == 0
    assert statistics.get_num_generated_candidates() == 0
    assert statistics.get_num_transferred_candidates() == 0
    assert statistics.get_communication_overhead() == 0
    assert statistics.get_num_registered_states() == 0
    assert statistics.get_idle_time() == timedelta(0)
    assert statistics.get_num_destination_lock_acquisitions() == 0
    assert statistics.get_destination_lock_wait_time() == timedelta(0)
    assert statistics.get_destination_lock_hold_time() == timedelta(0)
    for getter in (
        "get_state_storage_memory_usage",
        "get_action_bindings_memory_usage",
        "get_predicate_bindings_memory_usage",
        "get_axiom_bindings_memory_usage",
        "get_function_bindings_memory_usage",
    ):
        assert getattr(statistics, getter)() == 0
    statistics.increment_num_generated_successors()
    statistics.increment_num_expanded()
    statistics.increment_num_deadends()
    statistics.increment_num_pruned()
    statistics.increment_num_generated_candidates(transferred=False)
    statistics.increment_num_generated_candidates(transferred=True)
    assert statistics.get_num_generated_successors() == 1
    assert statistics.get_num_expanded() == 1
    assert statistics.get_num_deadends() == 1
    assert statistics.get_num_pruned() == 1
    assert statistics.get_num_generated_candidates() == 2
    assert statistics.get_num_transferred_candidates() == 1
    assert statistics.get_communication_overhead() == 0.5
    statistics.clear()
    assert statistics.get_num_generated_successors() == 0
    assert statistics.get_num_expanded() == 0
    assert statistics.get_num_deadends() == 0
    assert statistics.get_num_pruned() == 0
    assert statistics.get_num_generated_candidates() == 0
    assert statistics.get_num_transferred_candidates() == 0
    assert statistics.get_communication_overhead() == 0
    assert repr(statistics) == str(statistics)
    for label in (
        "Number of registered states",
        "State storage memory usage",
        "Action bindings memory usage",
        "Predicate bindings memory usage",
        "Axiom bindings memory usage",
        "Function bindings memory usage",
        "Number of generated candidates",
        "Number of transferred candidates",
        "Communication overhead",
        "Destination lock acquisitions",
        "Destination lock wait time",
        "Destination lock hold time",
    ):
        assert label in str(statistics)

    snapshot = planning.ProgressStatisticsSnapshot(1, 2, 3, 4)
    assert snapshot.get_num_generated_candidates() == 0
    assert snapshot.get_num_transferred_candidates() == 0
    snapshot = planning.ProgressStatisticsSnapshot(1, 2, 3, 4, num_generated_candidates=9, num_transferred_candidates=5)

    assert snapshot.get_num_generated_successors() == 1
    assert snapshot.get_num_expanded() == 2
    assert snapshot.get_num_deadends() == 3
    assert snapshot.get_num_pruned() == 4
    assert snapshot.get_num_generated_candidates() == 9
    assert snapshot.get_num_transferred_candidates() == 5
    assert repr(snapshot) == str(snapshot)
    assert "generated successors" in str(snapshot)
    assert "deadend" in str(snapshot)
    assert "Number of generated candidates at snapshot: 9" in str(snapshot)
    assert "Number of transferred candidates at snapshot: 5" in str(snapshot)

    progress_statistics = planning.ProgressStatistics()

    assert progress_statistics.get_snapshots() == []
    assert progress_statistics.empty()
    assert progress_statistics.size() == 0
    assert not hasattr(progress_statistics, "add_snap_shot")
    statistics.increment_num_generated_candidates(transferred=False)
    statistics.increment_num_generated_candidates(transferred=True)
    progress_statistics.add_snapshot(statistics)
    statistics.increment_num_generated_candidates(transferred=False)
    progress_statistics.add_snapshot(statistics)
    statistics.clear()
    assert len(progress_statistics.get_snapshots()) == 2
    assert progress_statistics.size() == 2
    assert not progress_statistics.empty()
    assert progress_statistics.get_snapshots()[0].get_num_generated_successors() == 0
    assert progress_statistics.get_snapshots()[1].get_num_generated_successors() == 0
    assert progress_statistics.get_snapshots()[0].get_num_generated_candidates() == 2
    assert progress_statistics.get_snapshots()[0].get_num_transferred_candidates() == 1
    assert progress_statistics.get_snapshots()[1].get_num_generated_candidates() == 3
    assert progress_statistics.get_snapshots()[1].get_num_transferred_candidates() == 1
    assert "Number of generated candidates at last snapshot: 3" in str(progress_statistics)
    assert "Number of transferred candidates at last snapshot: 1" in str(progress_statistics)
    progress_statistics.clear()
    assert progress_statistics.get_snapshots() == []
    assert progress_statistics.empty()
    assert progress_statistics.size() == 0

    progress_statistics = planning.ProgressStatistics()
    progress_statistics.add_snapshot(statistics)
    assert len(progress_statistics.get_snapshots()) == 1
    assert progress_statistics.get_snapshots()[0].get_num_generated_successors() == 0
    assert repr(progress_statistics) == str(progress_statistics)
    assert progress_statistics.get_snapshots()[0].get_num_generated_candidates() == 0
    assert progress_statistics.get_snapshots()[0].get_num_transferred_candidates() == 0
    assert "deadend" in str(progress_statistics)

    for task_module in (planning.ground, planning.lifted):
        for algorithm_module in (task_module.astar_eager, task_module.gbfs_lazy):
            event_handler = algorithm_module.DefaultEventHandler()
            assert not hasattr(event_handler, "on_expand_node")
            assert not hasattr(event_handler, "get_statistics")

        for event_handler in (task_module.brfs.DefaultEventHandler(), task_module.brfs.DefaultEventHandler(0)):
            assert isinstance(event_handler.get_progress_statistics(), planning.ProgressStatistics)
            assert not hasattr(event_handler, "on_expand_node")

        for event_handler in (task_module.iw.DefaultEventHandler(), task_module.iw.DefaultEventHandler(0)):
            assert isinstance(event_handler.get_statistics(), task_module.iw.Statistics)

        for event_handler in (task_module.siw.DefaultEventHandler(), task_module.siw.DefaultEventHandler(0)):
            assert isinstance(event_handler.get_statistics(), task_module.siw.Statistics)


def test_default_event_handler_statistics_keep_handlers_alive() -> None:
    for task_module in (planning.ground, planning.lifted):
        event_handler = task_module.brfs.DefaultEventHandler()
        progress_statistics = event_handler.get_progress_statistics()

        del event_handler
        gc.collect()

        assert progress_statistics.empty()

        event_handler = task_module.iw.DefaultEventHandler()
        statistics = event_handler.get_statistics()

        del event_handler
        gc.collect()

        assert statistics.get_solution_arity() is None

        event_handler = task_module.siw.DefaultEventHandler()
        statistics = event_handler.get_statistics()

        del event_handler
        gc.collect()

        assert statistics.get_num_solved_subsearches() == 0


def test_algorithm_statistics_are_constructible_and_mutable() -> None:
    for task_module in (planning.ground, planning.lifted):
        iw_statistics = task_module.iw.Statistics()

        assert iw_statistics.get_solution_arity() is None
        assert repr(iw_statistics) == str(iw_statistics)

        iw_statistics.set_solution_arity(3)
        assert iw_statistics.get_solution_arity() == 3

        iw_statistics.clear()
        assert iw_statistics.get_solution_arity() is None

        siw_statistics = task_module.siw.Statistics()

        assert siw_statistics.get_maximum_effective_width() is None
        assert siw_statistics.get_average_effective_width() is None
        assert siw_statistics.get_num_solved_subsearches() == 0
        assert repr(siw_statistics) == str(siw_statistics)

        siw_statistics.add_effective_width(2)
        siw_statistics.add_effective_width(4)

        assert siw_statistics.get_maximum_effective_width() == 4
        assert siw_statistics.get_average_effective_width() == 3.0
        assert siw_statistics.get_num_solved_subsearches() == 2
        assert "effective width" in str(siw_statistics)

        siw_statistics.clear()
        assert siw_statistics.get_maximum_effective_width() is None
        assert siw_statistics.get_average_effective_width() is None
        assert siw_statistics.get_num_solved_subsearches() == 0


def test_nested_width_search_solvers_expose_expected_defaults() -> None:
    iw_max_arity = 5

    for task_module in (planning.ground, planning.lifted):
        iw_solver = task_module.iw.Solver()

        assert isinstance(iw_solver.brfs_solver, task_module.brfs.Solver)
        assert iw_solver.brfs_solver.task is None
        assert iw_solver.brfs_solver.state_repository is None
        assert iw_solver.brfs_solver.axiom_evaluator is None
        assert iw_solver.brfs_solver.successor_generator is None
        assert iw_solver.max_arity == iw_max_arity
        assert iw_solver.options is not None

        siw_solver = task_module.siw.Solver()

        assert isinstance(siw_solver.iw_solver, task_module.iw.Solver)
        assert isinstance(siw_solver.iw_solver.brfs_solver, task_module.brfs.Solver)
        assert siw_solver.iw_solver.brfs_solver.task is None
        assert siw_solver.iw_solver.brfs_solver.state_repository is None
        assert siw_solver.iw_solver.brfs_solver.axiom_evaluator is None
        assert siw_solver.iw_solver.brfs_solver.successor_generator is None
        assert siw_solver.iw_solver.max_arity == iw_max_arity
        assert siw_solver.options is not None


def test_heuristics_without_a_relaxed_plan_return_none() -> None:
    for task_module in (planning.ground, planning.lifted):
        assert task_module.BlindHeuristic().get_relaxed_plan() is None


def test_search_result_exposes_all_result_fields() -> None:
    for task_module in (planning.ground, planning.lifted):
        result = task_module.SearchResult()

        assert result.status == planning.SearchStatus.IN_PROGRESS
        assert result.plan is None
        assert result.goal_node is None
        assert result.cycle_range is None
        assert isinstance(result.statistics, planning.Statistics)
        assert result.worker_statistics == []
        assert result.statistics.get_num_generated_successors() == 0

        result.status = planning.SearchStatus.CYCLE
        result.cycle_range = (1, 3)
        result.statistics.increment_num_generated_successors()

        assert result.status == planning.SearchStatus.CYCLE
        assert result.cycle_range == (1, 3)
        assert result.statistics.get_num_generated_successors() == 1


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


def test_search_budget_defaults_arguments_and_mutable_fields() -> None:
    unlimited = planning.SearchBudget()
    assert unlimited.max_num_states is None
    assert unlimited.max_time is None
    assert planning.SearchBudget(None, None).max_num_states is None
    assert planning.SearchBudget(max_num_states=None).max_num_states is None
    assert planning.SearchBudget(0).max_num_states == 0

    positional = planning.SearchBudget(50, timedelta(milliseconds=250))
    keyword = planning.SearchBudget(max_num_states=50, max_time=timedelta(milliseconds=250))
    assert positional.max_num_states == keyword.max_num_states == 50
    assert positional.max_time == keyword.max_time == timedelta(milliseconds=250)
    assert planning.SearchBudget(max_time=timedelta(seconds=2)).max_num_states == unlimited.max_num_states
    assert planning.SearchBudget(50).max_time is None

    positional.max_num_states = 75
    positional.max_time = None
    assert positional.max_num_states == 75
    assert positional.max_time is None
    positional.max_num_states = None
    assert positional.max_num_states is None
    positional.max_time = timedelta(seconds=3)
    assert positional.max_time == timedelta(seconds=3)


def test_algorithm_options_copy_budgets_and_keep_mutable_members_local() -> None:
    for task_module in (planning.ground, planning.lifted):
        for algorithm in (task_module.brfs, task_module.astar_eager, task_module.gbfs_lazy, task_module.iw):
            budget = planning.SearchBudget(50, timedelta(seconds=2))
            first, second = algorithm.Options(), algorithm.Options()
            first.search_budget = budget
            second.search_budget = budget

            budget.max_num_states = 75
            budget.max_time = None
            assert first.search_budget.max_num_states == second.search_budget.max_num_states == 50
            assert first.search_budget.max_time == second.search_budget.max_time == timedelta(seconds=2)

            first.search_budget.max_num_states = 100
            first.search_budget.max_time = timedelta(seconds=4)
            assert first.search_budget.max_num_states == 100
            assert first.search_budget.max_time == timedelta(seconds=4)
            assert second.search_budget.max_num_states == 50
            assert second.search_budget.max_time == timedelta(seconds=2)


def test_algorithm_options_are_default_constructible_with_expected_fields() -> None:
    ygg_uint_max = (1 << 32) - 1
    expected_defaults_by_algorithm = {
        "astar_eager": {
            "start_node": None,
            "event_handler": None,
            "pruning_strategy": None,
            "goal_strategy": None,
            "cost_mode": planning.CostMode.GENERAL,
            "num_search_workers": 1,
            "dist_hash_mode": planning.DistHashMode.RANDOM,
            "parallel_search_mode": planning.ParallelSearchMode.SYNCHRONOUS,
            "collect_destination_lock_statistics": False,
            "random_seed": 0,
            "shuffle_labeled_succ_nodes": False,
        },
        "brfs": {
            "start_node": None,
            "event_handler": None,
            "pruning_strategy": None,
            "goal_strategy": None,
            "num_search_workers": 1,
            "dist_hash_mode": planning.DistHashMode.RANDOM,
            "collect_destination_lock_statistics": False,
            "random_seed": 0,
            "shuffle_labeled_succ_nodes": False,
        },
        "gbfs_lazy": {
            "start_node": None,
            "event_handler": None,
            "pruning_strategy": None,
            "goal_strategy": None,
            "cost_mode": planning.CostMode.GENERAL,
            "use_preferred_actions": True,
            "boost_preferred_queue": 1000,
            "num_search_workers": 1,
            "dist_hash_mode": planning.DistHashMode.RANDOM,
            "collect_destination_lock_statistics": False,
            "random_seed": 0,
            "shuffle_labeled_succ_nodes": False,
        },
        "iw": {
            "start_node": None,
            "event_handler": None,
            "goal_strategy": None,
            "random_seed": 0,
            "shuffle_labeled_succ_nodes": False,
        },
        "siw": {
            "start_node": None,
            "event_handler": None,
            "subgoal_strategy": None,
            "goal_strategy": None,
            "max_num_subsearches": ygg_uint_max,
        },
    }

    expected_fields_by_algorithm = {
        "astar_eager": (
            "start_node",
            "event_handler",
            "pruning_strategy",
            "goal_strategy",
            "search_budget",
            "cost_mode",
            "num_search_workers",
            "dist_hash_mode",
            "parallel_search_mode",
            "collect_destination_lock_statistics",
            "random_seed",
            "shuffle_labeled_succ_nodes",
        ),
        "brfs": (
            "start_node",
            "event_handler",
            "pruning_strategy",
            "goal_strategy",
            "search_budget",
            "num_search_workers",
            "dist_hash_mode",
            "collect_destination_lock_statistics",
            "random_seed",
            "shuffle_labeled_succ_nodes",
        ),
        "gbfs_lazy": (
            "start_node",
            "event_handler",
            "pruning_strategy",
            "goal_strategy",
            "search_budget",
            "cost_mode",
            "use_preferred_actions",
            "boost_preferred_queue",
            "num_search_workers",
            "dist_hash_mode",
            "collect_destination_lock_statistics",
            "random_seed",
            "shuffle_labeled_succ_nodes",
        ),
        "iw": (
            "start_node",
            "event_handler",
            "goal_strategy",
            "search_budget",
            "random_seed",
            "shuffle_labeled_succ_nodes",
        ),
        "siw": (
            "start_node",
            "event_handler",
            "subgoal_strategy",
            "goal_strategy",
            "max_num_subsearches",
        ),
    }

    for task_module in (planning.ground, planning.lifted):
        for algorithm_name, expected_fields in expected_fields_by_algorithm.items():
            options = getattr(task_module, algorithm_name).Options()

            for field in expected_fields:
                assert hasattr(options, field)

            for field, expected_default in expected_defaults_by_algorithm[algorithm_name].items():
                assert getattr(options, field) == expected_default

            if algorithm_name != "siw":
                assert isinstance(options.search_budget, planning.SearchBudget)
                assert options.search_budget.max_num_states is None
                assert options.search_budget.max_time is None
                assert not hasattr(options, "max_num_states")
                assert not hasattr(options, "max_time")


def _make_gripper_tasks() -> tuple[planning.ground.Task, planning.lifted.Task]:
    parser_options = ParserOptions()
    parser = Parser(str(GRIPPER.domain_path), parser_options)
    formalism_task = parser.parse_task(
        str(GRIPPER.task_path),
        parser_options,
    )

    lifted_task = planning.lifted.Task(formalism_task)
    result = lifted_task.instantiate_ground_task(
        ExecutionContext(1),
        planning.lifted.GroundTaskInstantiationOptions(),
    )

    return result.task, lifted_task


SuccessorT = TypeVar("SuccessorT")


def _check_callback_visits(
    expected: Sequence[SuccessorT],
    visit: Callable[[Callable[[SuccessorT], bool]], bool],
    key: Callable[[SuccessorT], object],
) -> None:
    seen: list[SuccessorT] = []

    def record_all(successor: SuccessorT) -> bool:
        seen.append(successor)
        return True

    def record_first(successor: SuccessorT) -> bool:
        seen.append(successor)
        return False

    assert visit(record_all)
    assert [key(successor) for successor in seen] == [key(successor) for successor in expected]
    seen.clear()
    assert visit(record_first) == (not expected)
    assert [key(successor) for successor in seen] == [key(successor) for successor in expected[:1]]


def _fail_callback(_successor: object) -> Never:
    msg = "callback failure"
    raise RuntimeError(msg)


def _continue_callback(_successor: object) -> bool:
    return True


def _make_components_ground(
    task: planning.ground.Task, *, concurrent: bool = False
) -> tuple[planning.ground.StateRepository, planning.ground.AxiomEvaluator, planning.ground.SuccessorGenerator]:
    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.ground.AxiomEvaluatorFactory().create(task, execution_context)
    state_repository_factory = planning.ground.StateRepositoryFactory()
    state_repository = (
        state_repository_factory.create_concurrent(task) if concurrent else state_repository_factory.create(task)
    )
    successor_generator = planning.ground.SuccessorGeneratorFactory().create(task, execution_context)
    return state_repository, axiom_evaluator, successor_generator


def _make_components_lifted(
    task: planning.lifted.Task, *, concurrent: bool = False
) -> tuple[planning.lifted.StateRepository, planning.lifted.AxiomEvaluator, planning.lifted.SuccessorGenerator]:
    execution_context = ExecutionContext(1)
    axiom_evaluator = planning.lifted.AxiomEvaluatorFactory().create(task, execution_context)
    state_repository_factory = planning.lifted.StateRepositoryFactory()
    state_repository = (
        state_repository_factory.create_concurrent(task) if concurrent else state_repository_factory.create(task)
    )
    successor_generator = planning.lifted.SuccessorGeneratorFactory().create(task, execution_context)
    return state_repository, axiom_evaluator, successor_generator


def _check_lazy_gbfs_relaxed_plan_requests_ground(task: planning.ground.Task) -> None:
    relaxed_plans: tuple[set[formalism_planning.ActionBinding] | None, ...] = (set(), None)
    for plan in relaxed_plans:

        class RelaxedPlanHeuristic(planning.ground.Heuristic):
            def __init__(self, plan: set[formalism_planning.ActionBinding] | None) -> None:
                super().__init__()
                self.plan = plan
                self.num_plan_requests = 0

            @override
            def set_goal(self, goal: formalism_planning.GroundConjunctiveCondition) -> None:
                pass

            @override
            def evaluate(self, state: planning.ground.State) -> float:
                return 0.0

            def make_worker(self, _execution_context: ExecutionContext) -> Self:
                return self

            @override
            def get_relaxed_plan(self) -> set[formalism_planning.ActionBinding] | None:
                self.num_plan_requests += 1
                return self.plan

        heuristic = RelaxedPlanHeuristic(plan)
        state_repository, axiom_evaluator, successor_generator = _make_components_ground(task)
        result = planning.ground.gbfs_lazy.find_solution(
            task,
            state_repository,
            axiom_evaluator,
            successor_generator,
            heuristic,
            planning.ground.gbfs_lazy.Options(),
        )

        assert result.status == planning.SearchStatus.SOLVED
        assert heuristic.num_plan_requests == result.statistics.get_num_expanded()


def _check_lazy_gbfs_relaxed_plan_requests_lifted(task: planning.lifted.Task) -> None:
    relaxed_plans: tuple[set[formalism_planning.ActionBinding] | None, ...] = (set(), None)
    for plan in relaxed_plans:

        class RelaxedPlanHeuristic(planning.lifted.Heuristic):
            def __init__(self, plan: set[formalism_planning.ActionBinding] | None) -> None:
                super().__init__()
                self.plan = plan
                self.num_plan_requests = 0

            @override
            def set_goal(self, goal: formalism_planning.GroundConjunctiveCondition) -> None:
                pass

            @override
            def evaluate(self, state: planning.lifted.State) -> float:
                return 0.0

            def make_worker(self, _execution_context: ExecutionContext) -> Self:
                return self

            @override
            def get_relaxed_plan(self) -> set[formalism_planning.ActionBinding] | None:
                self.num_plan_requests += 1
                return self.plan

        heuristic = RelaxedPlanHeuristic(plan)
        state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task)
        result = planning.lifted.gbfs_lazy.find_solution(
            task,
            state_repository,
            axiom_evaluator,
            successor_generator,
            heuristic,
            planning.lifted.gbfs_lazy.Options(),
        )

        assert result.status == planning.SearchStatus.SOLVED
        assert heuristic.num_plan_requests == result.statistics.get_num_expanded()


def test_lazy_gbfs_reads_the_relaxed_plan_of_python_heuristics() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_lazy_gbfs_relaxed_plan_requests_ground(ground_task)
    _check_lazy_gbfs_relaxed_plan_requests_lifted(lifted_task)


def _check_parallel_search_ground(task: planning.ground.Task) -> None:
    class EventHandler(planning.ground.gbfs_lazy.EventHandler):
        def __init__(self) -> None:
            super().__init__()
            self.end_status: planning.SearchStatus | None = None
            self.num_expanded: int | None = None
            self.solved = False

        @override
        def on_start_search(self, node: planning.ground.Node, h_value: float) -> None:
            pass

        @override
        def on_new_best_h_value(self, h_value: float) -> None:
            pass

        @override
        def on_end_search(self, status: planning.SearchStatus, statistics: planning.Statistics) -> None:
            self.end_status = status
            self.num_expanded = statistics.get_num_expanded()

        @override
        def on_solved(self, plan: planning.ground.Plan) -> None:
            self.solved = True

    state_repository, axiom_evaluator, successor_generator = _make_components_ground(task, concurrent=True)
    event_handler = EventHandler()
    options = planning.ground.gbfs_lazy.Options()
    options.event_handler = event_handler
    options.num_search_workers = 2
    options.collect_destination_lock_statistics = True

    result = planning.ground.gbfs_lazy.find_solution(
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
    assert result.goal_node.get_state().get_state_repository() == state_repository
    assert result.statistics.get_num_expanded() > 0
    assert result.statistics.get_num_destination_lock_acquisitions() > 0
    assert event_handler.end_status == result.status
    assert event_handler.num_expanded == result.statistics.get_num_expanded()
    assert event_handler.solved
    assert not hasattr(event_handler, "on_expand_node")
    _assert_worker_statistics(result, 2)

    brfs_options = planning.ground.brfs.Options()
    brfs_options.num_search_workers = 2
    brfs_result = planning.ground.brfs.find_solution(
        task,
        state_repository,
        axiom_evaluator,
        successor_generator,
        brfs_options,
    )
    assert brfs_result.status == planning.SearchStatus.SOLVED
    assert brfs_result.goal_node is not None
    assert brfs_result.goal_node.get_state().get_state_repository() == state_repository
    _assert_worker_statistics(brfs_result, 2)

    astar_options = planning.ground.astar_eager.Options()
    astar_options.num_search_workers = 2
    for mode in (planning.ParallelSearchMode.SYNCHRONOUS, planning.ParallelSearchMode.ASYNCHRONOUS):
        astar_options.parallel_search_mode = mode
        astar_result = planning.ground.astar_eager.find_solution(
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
    class EventHandler(planning.lifted.gbfs_lazy.EventHandler):
        def __init__(self) -> None:
            super().__init__()
            self.end_status: planning.SearchStatus | None = None
            self.num_expanded: int | None = None
            self.solved = False

        @override
        def on_start_search(self, node: planning.lifted.Node, h_value: float) -> None:
            pass

        @override
        def on_new_best_h_value(self, h_value: float) -> None:
            pass

        @override
        def on_end_search(self, status: planning.SearchStatus, statistics: planning.Statistics) -> None:
            self.end_status = status
            self.num_expanded = statistics.get_num_expanded()

        @override
        def on_solved(self, plan: planning.lifted.Plan) -> None:
            self.solved = True

    state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task, concurrent=True)
    event_handler = EventHandler()
    options = planning.lifted.gbfs_lazy.Options()
    options.event_handler = event_handler
    options.num_search_workers = 2
    options.collect_destination_lock_statistics = True

    result = planning.lifted.gbfs_lazy.find_solution(
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
    assert result.goal_node.get_state().get_state_repository() == state_repository
    assert result.statistics.get_num_expanded() > 0
    assert result.statistics.get_num_destination_lock_acquisitions() > 0
    assert event_handler.end_status == result.status
    assert event_handler.num_expanded == result.statistics.get_num_expanded()
    assert event_handler.solved
    assert not hasattr(event_handler, "on_expand_node")
    _assert_worker_statistics(result, 2)

    brfs_options = planning.lifted.brfs.Options()
    brfs_options.num_search_workers = 2
    brfs_result = planning.lifted.brfs.find_solution(
        task,
        state_repository,
        axiom_evaluator,
        successor_generator,
        brfs_options,
    )
    assert brfs_result.status == planning.SearchStatus.SOLVED
    assert brfs_result.goal_node is not None
    assert brfs_result.goal_node.get_state().get_state_repository() == state_repository
    _assert_worker_statistics(brfs_result, 2)

    astar_options = planning.lifted.astar_eager.Options()
    astar_options.num_search_workers = 2
    for mode in (planning.ParallelSearchMode.SYNCHRONOUS, planning.ParallelSearchMode.ASYNCHRONOUS):
        astar_options.parallel_search_mode = mode
        astar_result = planning.lifted.astar_eager.find_solution(
            task,
            state_repository,
            axiom_evaluator,
            successor_generator,
            planning.lifted.BlindHeuristic(),
            astar_options,
        )
        assert astar_result.status == planning.SearchStatus.SOLVED
        _assert_worker_statistics(astar_result, 2)


def test_parallel_search_is_available_through_python_bindings() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_parallel_search_ground(ground_task)
    _check_parallel_search_lifted(lifted_task)


def _check_null_python_heuristic_workers_ground(task: planning.ground.Task) -> None:
    class NullWorkerHeuristic(planning.ground.Heuristic):
        def __init__(self) -> None:
            super().__init__()

        @override
        def set_goal(self, goal: formalism_planning.GroundConjunctiveCondition) -> None:
            pass

        @override
        def evaluate(self, state: planning.ground.State) -> float:
            return 0.0

        def make_worker(self, _execution_context: ExecutionContext) -> None:
            return None

    state_repository, axiom_evaluator, successor_generator = _make_components_ground(task)
    options = planning.ground.astar_eager.Options()
    options.num_search_workers = 2

    with pytest.raises(ValueError, match="heuristic does not support worker construction"):
        planning.ground.astar_eager.find_solution(
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
        def set_goal(self, goal: formalism_planning.GroundConjunctiveCondition) -> None:
            pass

        @override
        def evaluate(self, state: planning.lifted.State) -> float:
            return 0.0

        def make_worker(self, _execution_context: ExecutionContext) -> None:
            return None

    state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task)
    options = planning.lifted.astar_eager.Options()
    options.num_search_workers = 2

    with pytest.raises(ValueError, match="heuristic does not support worker construction"):
        planning.lifted.astar_eager.find_solution(
            task,
            state_repository,
            axiom_evaluator,
            successor_generator,
            NullWorkerHeuristic(),
            options,
        )


def test_parallel_search_rejects_null_python_heuristic_workers() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_null_python_heuristic_workers_ground(ground_task)
    _check_null_python_heuristic_workers_lifted(lifted_task)


def _check_python_goal_and_pruning_workers_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_ground(task, concurrent=True)
    initial_state = state_repository.get_initial_state(axiom_evaluator)
    fluent_variables = [fact.get_variable() for fact in initial_state.fluent_facts()]
    factory_indices: dict[str, list[tuple[type[object], int]]] = {"goal": [], "pruning": []}
    worker_ids: dict[str, set[int]] = {"goal": set(), "pruning": set()}
    callback_indices: dict[str, list[int | None]] = {"goal": [], "pruning": []}
    retained_snapshots: list[tuple[planning.ground.State, tuple[formalism_planning.FDRValue, ...]]] = []

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
        def is_dynamic_goal_satisfied(self, seed_state: planning.ground.State, state: planning.ground.State) -> bool:
            callback_indices["goal"].append(self.worker_index)
            if len(callback_indices["goal"]) > 2 and not retained_snapshots:
                retained_snapshots.append((state, tuple(state.get(variable) for variable in fluent_variables)))
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
        def should_prune_state(self, state: planning.ground.State) -> bool:
            callback_indices["pruning"].append(self.worker_index)
            return False

        @override
        def should_prune_successor_state(
            self, state: planning.ground.State, succ_state: planning.ground.State, is_new_succ_state: bool
        ) -> bool:
            callback_indices["pruning"].append(self.worker_index)
            return False

    options = planning.ground.brfs.Options()
    options.num_search_workers = 2
    options.goal_strategy = PythonGoalStrategy()
    options.pruning_strategy = PythonPruningStrategy()

    result = planning.ground.brfs.find_solution(task, state_repository, axiom_evaluator, successor_generator, options)

    assert result.status == planning.SearchStatus.EXHAUSTED
    for strategy in ("goal", "pruning"):
        assert factory_indices[strategy] == [(planning.WorkerIndex, 1)]
        assert len(worker_ids[strategy]) == 1
        assert set(callback_indices[strategy]) == {None, 1}

    assert "Owning read-only state snapshot" in (planning.ground.StateBuilder.__doc__ or "")
    assert retained_snapshots
    snapshot, values = retained_snapshots[0]
    gc.collect()
    assert tuple(snapshot.get(variable) for variable in fluent_variables) == values


def _check_python_goal_and_pruning_workers_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task, concurrent=True)
    initial_state = state_repository.get_initial_state(axiom_evaluator)
    fluent_variables = [fact.get_variable() for fact in initial_state.fluent_facts()]
    factory_indices: dict[str, list[tuple[type[object], int]]] = {"goal": [], "pruning": []}
    worker_ids: dict[str, set[int]] = {"goal": set(), "pruning": set()}
    callback_indices: dict[str, list[int | None]] = {"goal": [], "pruning": []}
    retained_snapshots: list[tuple[planning.lifted.State, tuple[formalism_planning.FDRValue, ...]]] = []

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
        def is_dynamic_goal_satisfied(self, seed_state: planning.lifted.State, state: planning.lifted.State) -> bool:
            callback_indices["goal"].append(self.worker_index)
            if len(callback_indices["goal"]) > 2 and not retained_snapshots:
                retained_snapshots.append((state, tuple(state.get(variable) for variable in fluent_variables)))
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
        def should_prune_state(self, state: planning.lifted.State) -> bool:
            callback_indices["pruning"].append(self.worker_index)
            return False

        @override
        def should_prune_successor_state(
            self, state: planning.lifted.State, succ_state: planning.lifted.State, is_new_succ_state: bool
        ) -> bool:
            callback_indices["pruning"].append(self.worker_index)
            return False

    options = planning.lifted.brfs.Options()
    options.num_search_workers = 2
    options.goal_strategy = PythonGoalStrategy()
    options.pruning_strategy = PythonPruningStrategy()

    result = planning.lifted.brfs.find_solution(task, state_repository, axiom_evaluator, successor_generator, options)

    assert result.status == planning.SearchStatus.EXHAUSTED
    for strategy in ("goal", "pruning"):
        assert factory_indices[strategy] == [(planning.WorkerIndex, 1)]
        assert len(worker_ids[strategy]) == 1
        assert set(callback_indices[strategy]) == {None, 1}

    assert "Owning read-only state snapshot" in (planning.lifted.StateBuilder.__doc__ or "")
    assert retained_snapshots
    snapshot, values = retained_snapshots[0]
    gc.collect()
    assert tuple(snapshot.get(variable) for variable in fluent_variables) == values


def test_parallel_search_constructs_python_goal_and_pruning_workers() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_python_goal_and_pruning_workers_ground(ground_task)
    _check_python_goal_and_pruning_workers_lifted(lifted_task)


def _check_missing_or_null_python_strategy_workers_ground(task: planning.ground.Task) -> None:
    class MissingWorkerGoalStrategy(planning.ground.GoalStrategy):
        @override
        def is_static_goal_satisfied(self, task: planning.ground.Task) -> bool:
            return True

        @override
        def is_dynamic_goal_satisfied(self, seed_state: planning.ground.State, state: planning.ground.State) -> bool:
            return False

    class MissingWorkerPruningStrategy(planning.ground.PruningStrategy):
        pass

    state_repository, axiom_evaluator, successor_generator = _make_components_ground(task)
    cases = (
        ("goal_strategy", MissingWorkerGoalStrategy(), "goal strategy does not support worker construction"),
        (
            "pruning_strategy",
            MissingWorkerPruningStrategy(),
            "pruning strategy does not support worker construction",
        ),
    )

    for field, strategy, error in cases:
        options = planning.ground.brfs.Options()
        options.num_search_workers = 2
        setattr(options, field, strategy)
        with pytest.raises(ValueError, match=error):
            planning.ground.brfs.find_solution(task, state_repository, axiom_evaluator, successor_generator, options)


def _check_missing_or_null_python_strategy_workers_lifted(task: planning.lifted.Task) -> None:
    class MissingWorkerGoalStrategy(planning.lifted.GoalStrategy):
        @override
        def is_static_goal_satisfied(self, task: planning.lifted.Task) -> bool:
            return True

        @override
        def is_dynamic_goal_satisfied(self, seed_state: planning.lifted.State, state: planning.lifted.State) -> bool:
            return False

    class MissingWorkerPruningStrategy(planning.lifted.PruningStrategy):
        pass

    state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task)
    cases = (
        ("goal_strategy", MissingWorkerGoalStrategy(), "goal strategy does not support worker construction"),
        (
            "pruning_strategy",
            MissingWorkerPruningStrategy(),
            "pruning strategy does not support worker construction",
        ),
    )

    for field, strategy, error in cases:
        options = planning.lifted.brfs.Options()
        options.num_search_workers = 2
        setattr(options, field, strategy)
        with pytest.raises(ValueError, match=error):
            planning.lifted.brfs.find_solution(task, state_repository, axiom_evaluator, successor_generator, options)


def test_parallel_search_rejects_missing_or_null_python_strategy_workers() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_missing_or_null_python_strategy_workers_ground(ground_task)
    _check_missing_or_null_python_strategy_workers_lifted(lifted_task)


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


def test_cost_mode_is_bound_for_cost_sensitive_algorithms() -> None:
    assert planning.CostMode.UNIT != planning.CostMode.GENERAL

    for task_module in (planning.ground, planning.lifted):
        for algorithm_module in (task_module.astar_eager, task_module.gbfs_lazy):
            options = algorithm_module.Options()

            assert options.cost_mode == planning.CostMode.GENERAL
            options.cost_mode = planning.CostMode.UNIT
            assert options.cost_mode == planning.CostMode.UNIT


def test_parallel_astar_search_mode_is_bound() -> None:
    assert planning.ParallelSearchMode.SYNCHRONOUS != planning.ParallelSearchMode.ASYNCHRONOUS

    for task_module in (planning.ground, planning.lifted):
        options = task_module.astar_eager.Options()
        assert options.parallel_search_mode == planning.ParallelSearchMode.SYNCHRONOUS
        options.parallel_search_mode = planning.ParallelSearchMode.ASYNCHRONOUS
        assert options.parallel_search_mode == planning.ParallelSearchMode.ASYNCHRONOUS


def test_state_repository_concurrency_is_bound() -> None:
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        factory = task_module.StateRepositoryFactory()
        assert not factory.create(task).is_concurrent()
        assert factory.create_concurrent(task).is_concurrent()


def test_dist_hash_mode_is_bound() -> None:
    assert planning.DistHashMode.RANDOM != planning.DistHashMode.LMCUT

    for task_module in (planning.ground, planning.lifted):
        for algorithm_module in (
            task_module.astar_eager,
            task_module.brfs,
            task_module.gbfs_lazy,
        ):
            options = algorithm_module.Options()
            assert options.dist_hash_mode == planning.DistHashMode.RANDOM
            options.dist_hash_mode = planning.DistHashMode.LMCUT
            assert options.dist_hash_mode == planning.DistHashMode.LMCUT


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


def test_state_repository_create_state_uses_plural_fluent_facts_argument() -> None:
    docstring = planning.ground.StateRepository.create_state.__doc__ or ""

    assert "fluent_facts" in docstring
    assert "fluent_fact:" not in docstring


def test_state_get_uses_fluent_variable_argument_name() -> None:
    docstring = planning.ground.State.get.__doc__ or ""

    assert "fluent_variable" in docstring
    assert "fluent_fact:" not in docstring


def _check_state_iterators_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, _ = _make_components_ground(task)
    state = state_repository.get_initial_state(axiom_evaluator)

    static_atoms = state.static_atoms()

    assert iter(static_atoms) is static_atoms
    first_static_atom = next(static_atoms)
    assert first_static_atom in list(state.static_atoms())
    assert len(list(static_atoms)) == 11
    assert list(static_atoms) == []
    assert len(list(state.fluent_facts())) == 5
    fluent_atoms = list(state.fluent_atoms())
    assert len(fluent_atoms) == 5
    assert all(state.test(atom) for atom in fluent_atoms)
    assert len(list(state.derived_atoms())) == 0
    assert len(list(state.static_fterm_values())) == 0
    assert len(list(state.fluent_fterm_values())) == 0

    static_atoms = state.static_atoms()
    del state
    gc.collect()
    assert len(list(static_atoms)) == 12


def _check_state_iterators_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, _ = _make_components_lifted(task)
    state = state_repository.get_initial_state(axiom_evaluator)

    static_atoms = state.static_atoms()

    assert iter(static_atoms) is static_atoms
    first_static_atom = next(static_atoms)
    assert first_static_atom in list(state.static_atoms())
    assert len(list(static_atoms)) == 11
    assert list(static_atoms) == []
    assert len(list(state.fluent_facts())) == 5
    fluent_atoms = list(state.fluent_atoms())
    assert len(fluent_atoms) == 5
    assert all(state.test(atom) for atom in fluent_atoms)
    assert len(list(state.derived_atoms())) == 0
    assert len(list(state.static_fterm_values())) == 0
    assert len(list(state.fluent_fterm_values())) == 0

    static_atoms = state.static_atoms()
    del state
    gc.collect()
    assert len(list(static_atoms)) == 12


def test_state_iterable_methods_return_stable_iterators() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_state_iterators_ground(ground_task)
    _check_state_iterators_lifted(lifted_task)


def _check_packed_state_roundtrip_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_ground(task)
    state = state_repository.get_initial_state(axiom_evaluator)
    state_index = state.get_index()
    state_text = str(state)
    packed = state.pack()

    assert type(packed) is planning.ground.PackedState
    assert packed.get_index() == state_index
    assert packed.get_state_repository() == state_repository
    assert packed.unpack() == state
    assert packed == state.pack()
    assert hash(packed) == hash(state.pack())

    del state, state_repository, axiom_evaluator, successor_generator
    gc.collect()

    unpacked = packed.unpack()
    assert type(unpacked) is planning.ground.State
    assert unpacked.get_index() == state_index
    assert str(unpacked) == state_text
    assert unpacked.pack() == packed

    del packed
    gc.collect()
    assert str(unpacked) == state_text


def _check_packed_state_roundtrip_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task)
    state = state_repository.get_initial_state(axiom_evaluator)
    state_index = state.get_index()
    state_text = str(state)
    packed = state.pack()

    assert type(packed) is planning.lifted.PackedState
    assert packed.get_index() == state_index
    assert packed.get_state_repository() == state_repository
    assert packed.unpack() == state
    assert packed == state.pack()
    assert hash(packed) == hash(state.pack())

    del state, state_repository, axiom_evaluator, successor_generator
    gc.collect()

    unpacked = packed.unpack()
    assert type(unpacked) is planning.lifted.State
    assert unpacked.get_index() == state_index
    assert str(unpacked) == state_text
    assert unpacked.pack() == packed

    del packed
    gc.collect()
    assert str(unpacked) == state_text


def test_packed_state_roundtrip_retains_repository() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_packed_state_roundtrip_ground(ground_task)
    _check_packed_state_roundtrip_lifted(lifted_task)


def _check_packed_nodes_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_ground(task)
    initial_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)
    successor = successor_generator.get_labeled_successor_nodes(initial_node, state_repository, axiom_evaluator)[0]
    node = planning.ground.Node(successor.node.get_state(), 17.5)
    labeled = planning.ground.LabeledNode(successor.label, node)
    state_text = str(node.get_state())
    label_text = str(labeled.label)
    packed = node.pack()
    packed_labeled = labeled.pack()

    assert type(packed) is planning.ground.PackedNode
    assert type(packed.get_state()) is planning.ground.PackedState
    assert packed.get_metric() == 17.5
    assert packed == planning.ground.PackedNode(node.get_state().pack(), 17.5)
    assert hash(packed) == hash(node.pack())
    assert packed.unpack() == node
    assert type(packed_labeled) is planning.ground.PackedLabeledNode
    assert packed_labeled.node == packed
    assert packed_labeled.label == labeled.label
    assert planning.ground.PackedLabeledNode(labeled.label, packed).unpack().node == node
    assert repr(packed) == str(packed)
    assert repr(packed_labeled) == str(packed_labeled)

    del initial_node, successor, node, labeled, state_repository, axiom_evaluator, successor_generator
    gc.collect()

    assert str(packed.unpack().get_state()) == state_text
    unpacked_labeled = packed_labeled.unpack()
    del packed, packed_labeled
    gc.collect()
    assert type(unpacked_labeled) is planning.ground.LabeledNode
    assert unpacked_labeled.node.get_metric() == 17.5
    assert str(unpacked_labeled.node.get_state()) == state_text
    assert str(unpacked_labeled.label) == label_text


def _check_packed_nodes_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task)
    initial_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)
    successor = successor_generator.get_labeled_successor_nodes(initial_node, state_repository, axiom_evaluator)[0]
    node = planning.lifted.Node(successor.node.get_state(), 17.5)
    labeled = planning.lifted.LabeledNode(successor.label, node)
    state_text = str(node.get_state())
    label_text = str(labeled.label)
    packed = node.pack()
    packed_labeled = labeled.pack()

    assert type(packed) is planning.lifted.PackedNode
    assert type(packed.get_state()) is planning.lifted.PackedState
    assert packed.get_metric() == 17.5
    assert packed == planning.lifted.PackedNode(node.get_state().pack(), 17.5)
    assert hash(packed) == hash(node.pack())
    assert packed.unpack() == node
    assert type(packed_labeled) is planning.lifted.PackedLabeledNode
    assert packed_labeled.node == packed
    assert packed_labeled.label == labeled.label
    assert planning.lifted.PackedLabeledNode(labeled.label, packed).unpack().node == node
    assert repr(packed) == str(packed)
    assert repr(packed_labeled) == str(packed_labeled)

    del initial_node, successor, node, labeled, state_repository, axiom_evaluator, successor_generator
    gc.collect()

    assert str(packed.unpack().get_state()) == state_text
    unpacked_labeled = packed_labeled.unpack()
    del packed, packed_labeled
    gc.collect()
    assert type(unpacked_labeled) is planning.lifted.LabeledNode
    assert unpacked_labeled.node.get_metric() == 17.5
    assert str(unpacked_labeled.node.get_state()) == state_text
    assert str(unpacked_labeled.label) == label_text


def test_packed_nodes_preserve_state_metric_and_label() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_packed_nodes_ground(ground_task)
    _check_packed_nodes_lifted(lifted_task)


def _first_successor_label_ground(
    task: planning.ground.Task, *, packed: bool
) -> tuple[formalism_planning.ActionBinding, str]:
    state_repository, axiom_evaluator, successor_generator = _make_components_ground(task)
    initial_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)
    labeled = successor_generator.get_labeled_successor_nodes(initial_node, state_repository, axiom_evaluator)[0]
    label = labeled.pack().label if packed else labeled.label
    return label, str(label)


def _first_successor_label_lifted(
    task: planning.lifted.Task, *, packed: bool
) -> tuple[formalism_planning.ActionBinding, str]:
    state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task)
    initial_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)
    labeled = successor_generator.get_labeled_successor_nodes(initial_node, state_repository, axiom_evaluator)[0]
    label = labeled.pack().label if packed else labeled.label
    return label, str(label)


@pytest.mark.parametrize("family", ["ground", "lifted"])
@pytest.mark.parametrize("packed", [False, True])
def test_labeled_node_label_keeps_owning_node_alive(family: str, packed: bool) -> None:
    def make_label() -> tuple[formalism_planning.ActionBinding, str]:
        ground_task, lifted_task = _make_gripper_tasks()
        if family == "ground":
            return _first_successor_label_ground(ground_task, packed=packed)
        return _first_successor_label_lifted(lifted_task, packed=packed)

    label, expected = make_label()
    gc.collect()
    assert str(label) == expected


def _check_create_state_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, _ = _make_components_ground(task)

    assert state_repository.num_states() == 0
    assert state_repository.memory_usage() >= 0

    initial_state = state_repository.get_initial_state(axiom_evaluator)
    recreated_state = state_repository.create_state(
        axiom_evaluator,
        list(initial_state.fluent_facts()),
        list(initial_state.fluent_fterm_values()),
    )
    value_recreated_state = state_repository.create_state(
        axiom_evaluator,
        [
            formalism_planning.FluentFDRFactData(fact.get_variable(), fact.get_value())
            for fact in initial_state.fluent_facts()
        ],
        [(fterm.get_index(), value) for fterm, value in initial_state.fluent_fterm_values()],
    )

    registered_state = state_repository.get_registered_state(initial_state.get_index())

    assert registered_state == initial_state
    assert registered_state.get_index() == initial_state.get_index()
    assert recreated_state == initial_state
    assert recreated_state.get_index() == initial_state.get_index()
    assert value_recreated_state == initial_state
    assert value_recreated_state.get_index() == initial_state.get_index()
    assert state_repository.num_states() == 1


def _check_create_state_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, _ = _make_components_lifted(task)

    assert state_repository.num_states() == 0
    assert state_repository.memory_usage() >= 0

    initial_state = state_repository.get_initial_state(axiom_evaluator)
    recreated_state = state_repository.create_state(
        axiom_evaluator,
        list(initial_state.fluent_facts()),
        list(initial_state.fluent_fterm_values()),
    )
    value_recreated_state = state_repository.create_state(
        axiom_evaluator,
        [
            formalism_planning.FluentFDRFactData(fact.get_variable(), fact.get_value())
            for fact in initial_state.fluent_facts()
        ],
        [(fterm.get_index(), value) for fterm, value in initial_state.fluent_fterm_values()],
    )

    registered_state = state_repository.get_registered_state(initial_state.get_index())

    assert registered_state == initial_state
    assert registered_state.get_index() == initial_state.get_index()
    assert recreated_state == initial_state
    assert recreated_state.get_index() == initial_state.get_index()
    assert value_recreated_state == initial_state
    assert value_recreated_state.get_index() == initial_state.get_index()
    assert state_repository.num_states() == 1


def test_state_repository_create_state_accepts_state_iterables() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_create_state_ground(ground_task)
    _check_create_state_lifted(lifted_task)


def _check_repr_matches_str_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_ground(task)
    state = state_repository.get_initial_state(axiom_evaluator)
    node = planning.ground.Node(state, 0.0)
    labeled_node = successor_generator.get_labeled_successor_nodes(node, state_repository, axiom_evaluator)[0]
    plan = planning.ground.Plan(node)

    assert repr(state) == str(state)
    assert repr(node) == str(node)
    assert repr(labeled_node) == str(labeled_node)
    assert repr(plan) == str(plan)
    assert plan.get_length() == 0
    assert plan.empty()


def _check_repr_matches_str_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task)
    state = state_repository.get_initial_state(axiom_evaluator)
    node = planning.lifted.Node(state, 0.0)
    labeled_node = successor_generator.get_labeled_successor_nodes(node, state_repository, axiom_evaluator)[0]
    plan = planning.lifted.Plan(node)

    assert repr(state) == str(state)
    assert repr(node) == str(node)
    assert repr(labeled_node) == str(labeled_node)
    assert repr(plan) == str(plan)
    assert plan.get_length() == 0
    assert plan.empty()


def test_planning_state_node_and_plan_repr_matches_str() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_repr_matches_str_ground(ground_task)
    _check_repr_matches_str_lifted(lifted_task)


def _check_node_equality_and_hash_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, _ = _make_components_ground(task)
    state = state_repository.get_initial_state(axiom_evaluator)

    first_node = planning.ground.Node(state, 1.0)
    second_node = planning.ground.Node(state, 1.0)
    different_metric_node = planning.ground.Node(state, 2.0)

    assert first_node == second_node
    assert hash(first_node) == hash(second_node)
    assert first_node != different_metric_node


def _check_node_equality_and_hash_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, _ = _make_components_lifted(task)
    state = state_repository.get_initial_state(axiom_evaluator)

    first_node = planning.lifted.Node(state, 1.0)
    second_node = planning.lifted.Node(state, 1.0)
    different_metric_node = planning.lifted.Node(state, 2.0)

    assert first_node == second_node
    assert hash(first_node) == hash(second_node)
    assert first_node != different_metric_node


def test_node_python_equality_and_hash_follow_state_and_metric() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_node_equality_and_hash_ground(ground_task)
    _check_node_equality_and_hash_lifted(lifted_task)


def _check_node_lookup_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_ground(task)
    initial_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)
    looked_up_node = successor_generator.get_node(state_repository, initial_node.get_state().get_index())

    assert successor_generator.get_index() == 0
    assert not hasattr(successor_generator, "get_state_repository")
    assert looked_up_node == initial_node
    assert successor_generator.get_packed_initial_node(state_repository, axiom_evaluator).unpack() == initial_node
    assert (
        successor_generator.get_packed_node(state_repository, initial_node.get_state().get_index()).unpack()
        == initial_node
    )


def _check_node_lookup_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task)
    initial_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)
    looked_up_node = successor_generator.get_node(state_repository, initial_node.get_state().get_index())

    assert successor_generator.get_index() == 0
    assert not hasattr(successor_generator, "get_state_repository")
    assert looked_up_node == initial_node
    assert successor_generator.get_packed_initial_node(state_repository, axiom_evaluator).unpack() == initial_node
    assert (
        successor_generator.get_packed_node(state_repository, initial_node.get_state().get_index()).unpack()
        == initial_node
    )


def test_successor_generator_accepts_explicit_repository_for_node_lookup() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_node_lookup_ground(ground_task)
    _check_node_lookup_lifted(lifted_task)


def _check_action_binding_api_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_ground(task)
    start_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)

    bindings = successor_generator.get_applicable_action_bindings(start_node)
    labeled_successor_nodes = successor_generator.get_labeled_successor_nodes(
        start_node, state_repository, axiom_evaluator
    )

    assert len(bindings) == len(labeled_successor_nodes)
    assert bindings

    binding = bindings[0]
    ground_action = successor_generator.ground_action(binding)
    binding_successor = successor_generator.get_successor_node(start_node, binding, state_repository, axiom_evaluator)
    ground_action_successor = successor_generator.get_successor_node(
        start_node, ground_action, state_repository, axiom_evaluator
    )

    assert type(binding) is formalism_planning.ActionBinding
    assert successor_generator.ground_action(binding) == ground_action
    assert ground_action.get_action().get_arity() == len(list(ground_action.get_objects()))
    assert binding_successor == ground_action_successor
    packed_binding_successor = successor_generator.get_packed_successor_node(
        start_node, binding, state_repository, axiom_evaluator
    )
    packed_action_successor = successor_generator.get_packed_successor_node(
        start_node, ground_action, state_repository, axiom_evaluator
    )
    assert packed_binding_successor.unpack() == binding_successor
    assert packed_action_successor.unpack() == binding_successor
    assert any(
        labeled_successor.label == binding and labeled_successor.node == binding_successor
        for labeled_successor in labeled_successor_nodes
    )


def _check_action_binding_api_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task)
    start_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)

    bindings = successor_generator.get_applicable_action_bindings(start_node)
    labeled_successor_nodes = successor_generator.get_labeled_successor_nodes(
        start_node, state_repository, axiom_evaluator
    )

    assert len(bindings) == len(labeled_successor_nodes)
    assert bindings

    binding = bindings[0]
    ground_action = successor_generator.ground_action(binding)
    binding_successor = successor_generator.get_successor_node(start_node, binding, state_repository, axiom_evaluator)
    ground_action_successor = successor_generator.get_successor_node(
        start_node, ground_action, state_repository, axiom_evaluator
    )

    assert type(binding) is formalism_planning.ActionBinding
    assert successor_generator.ground_action(binding) == ground_action
    assert ground_action.get_action().get_arity() == len(list(ground_action.get_objects()))
    assert binding_successor == ground_action_successor
    packed_binding_successor = successor_generator.get_packed_successor_node(
        start_node, binding, state_repository, axiom_evaluator
    )
    packed_action_successor = successor_generator.get_packed_successor_node(
        start_node, ground_action, state_repository, axiom_evaluator
    )
    assert packed_binding_successor.unpack() == binding_successor
    assert packed_action_successor.unpack() == binding_successor
    assert any(
        labeled_successor.label == binding and labeled_successor.node == binding_successor
        for labeled_successor in labeled_successor_nodes
    )


def test_successor_generators_expose_uniform_action_binding_api() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_action_binding_api_ground(ground_task)
    _check_action_binding_api_lifted(lifted_task)


def _check_action_schema_successors_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_ground(task)
    actions = list(task.get_task().get_domain().get_actions())
    initial_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)
    initial_successors = successor_generator.get_labeled_successor_nodes(
        initial_node, state_repository, axiom_evaluator
    )
    picked_node = next(
        successor.node
        for successor in initial_successors
        if successor.label.get_relation().get_original_name() == "pick"
    )

    for node in (initial_node, picked_node):
        all_successors = successor_generator.get_labeled_successor_nodes(node, state_repository, axiom_evaluator)
        all_bindings = successor_generator.get_applicable_action_bindings(node)
        packed = successor_generator.get_packed_labeled_successor_nodes(node, state_repository, axiom_evaluator)
        assert [(successor.label, successor.node.unpack()) for successor in packed] == [
            (successor.label, successor.node) for successor in all_successors
        ]
        packed_nodes = successor_generator.get_packed_successor_nodes(node, state_repository, axiom_evaluator)
        assert [successor.unpack() for successor in packed_nodes] == [successor.node for successor in all_successors]
        assert Counter(successor_generator.get_successor_nodes(node, state_repository, axiom_evaluator)) == Counter(
            successor.node for successor in all_successors
        )
        for action in actions:
            expected = {
                successor.label: successor.node
                for successor in all_successors
                if successor.label.get_relation() == action
            }
            selected = successor_generator.get_labeled_successor_nodes(node, action, state_repository, axiom_evaluator)
            assert {successor.label: successor.node for successor in selected} == expected
            assert set(successor_generator.get_applicable_action_bindings(node, action)) == {
                binding for binding in all_bindings if binding.get_relation() == action
            }
            assert Counter(
                successor_generator.get_successor_nodes(node, action, state_repository, axiom_evaluator)
            ) == Counter(expected.values())
            packed = successor_generator.get_packed_labeled_successor_nodes(
                node, action, state_repository, axiom_evaluator
            )
            assert {successor.label: successor.node.unpack() for successor in packed} == expected
            packed_nodes = successor_generator.get_packed_successor_nodes(
                node, action, state_repository, axiom_evaluator
            )
            assert Counter(successor.unpack() for successor in packed_nodes) == Counter(expected.values())
            if action.get_original_name() == "drop":
                assert bool(expected) == (node == picked_node)


def _check_action_schema_successors_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task)
    actions = list(task.get_task().get_domain().get_actions())
    initial_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)
    initial_successors = successor_generator.get_labeled_successor_nodes(
        initial_node, state_repository, axiom_evaluator
    )
    picked_node = next(
        successor.node
        for successor in initial_successors
        if successor.label.get_relation().get_original_name() == "pick"
    )

    for node in (initial_node, picked_node):
        all_successors = successor_generator.get_labeled_successor_nodes(node, state_repository, axiom_evaluator)
        all_bindings = successor_generator.get_applicable_action_bindings(node)
        packed = successor_generator.get_packed_labeled_successor_nodes(node, state_repository, axiom_evaluator)
        assert [(successor.label, successor.node.unpack()) for successor in packed] == [
            (successor.label, successor.node) for successor in all_successors
        ]
        packed_nodes = successor_generator.get_packed_successor_nodes(node, state_repository, axiom_evaluator)
        assert [successor.unpack() for successor in packed_nodes] == [successor.node for successor in all_successors]
        assert Counter(successor_generator.get_successor_nodes(node, state_repository, axiom_evaluator)) == Counter(
            successor.node for successor in all_successors
        )
        for action in actions:
            expected = {
                successor.label: successor.node
                for successor in all_successors
                if successor.label.get_relation() == action
            }
            selected = successor_generator.get_labeled_successor_nodes(node, action, state_repository, axiom_evaluator)
            assert {successor.label: successor.node for successor in selected} == expected
            assert set(successor_generator.get_applicable_action_bindings(node, action)) == {
                binding for binding in all_bindings if binding.get_relation() == action
            }
            assert Counter(
                successor_generator.get_successor_nodes(node, action, state_repository, axiom_evaluator)
            ) == Counter(expected.values())
            packed = successor_generator.get_packed_labeled_successor_nodes(
                node, action, state_repository, axiom_evaluator
            )
            assert {successor.label: successor.node.unpack() for successor in packed} == expected
            packed_nodes = successor_generator.get_packed_successor_nodes(
                node, action, state_repository, axiom_evaluator
            )
            assert Counter(successor.unpack() for successor in packed_nodes) == Counter(expected.values())
            if action.get_original_name() == "drop":
                assert bool(expected) == (node == picked_node)


def test_successor_generation_for_a_lifted_action_schema() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_action_schema_successors_ground(ground_task)
    _check_action_schema_successors_lifted(lifted_task)


def _check_unlabeled_successor_callbacks_ground(
    generator: planning.ground.SuccessorGenerator,
    node: planning.ground.Node,
    action: formalism_planning.Action | None,
    state_repository: planning.ground.StateRepository,
    axiom_evaluator: planning.ground.AxiomEvaluator,
) -> None:
    if action is None:
        _check_callback_visits(
            generator.get_successor_nodes(node, state_repository, axiom_evaluator),
            lambda callback: generator.for_each_successor_node(node, state_repository, axiom_evaluator, callback),
            lambda successor: successor,
        )
    else:
        _check_callback_visits(
            generator.get_successor_nodes(node, action, state_repository, axiom_evaluator),
            lambda callback: generator.for_each_successor_node(
                node, action, state_repository, axiom_evaluator, callback
            ),
            lambda successor: successor,
        )


def _check_unlabeled_successor_callbacks_lifted(
    generator: planning.lifted.SuccessorGenerator,
    node: planning.lifted.Node,
    action: formalism_planning.Action | None,
    state_repository: planning.lifted.StateRepository,
    axiom_evaluator: planning.lifted.AxiomEvaluator,
) -> None:
    if action is None:
        _check_callback_visits(
            generator.get_successor_nodes(node, state_repository, axiom_evaluator),
            lambda callback: generator.for_each_successor_node(node, state_repository, axiom_evaluator, callback),
            lambda successor: successor,
        )
    else:
        _check_callback_visits(
            generator.get_successor_nodes(node, action, state_repository, axiom_evaluator),
            lambda callback: generator.for_each_successor_node(
                node, action, state_repository, axiom_evaluator, callback
            ),
            lambda successor: successor,
        )


def _check_labeled_successor_callbacks_ground(
    generator: planning.ground.SuccessorGenerator,
    node: planning.ground.Node,
    action: formalism_planning.Action | None,
    state_repository: planning.ground.StateRepository,
    axiom_evaluator: planning.ground.AxiomEvaluator,
) -> None:
    if action is None:
        _check_callback_visits(
            generator.get_labeled_successor_nodes(node, state_repository, axiom_evaluator),
            lambda callback: generator.for_each_labeled_successor_node(
                node, state_repository, axiom_evaluator, callback
            ),
            lambda successor: (successor.label, successor.node),
        )
    else:
        _check_callback_visits(
            generator.get_labeled_successor_nodes(node, action, state_repository, axiom_evaluator),
            lambda callback: generator.for_each_labeled_successor_node(
                node, action, state_repository, axiom_evaluator, callback
            ),
            lambda successor: (successor.label, successor.node),
        )


def _check_labeled_successor_callbacks_lifted(
    generator: planning.lifted.SuccessorGenerator,
    node: planning.lifted.Node,
    action: formalism_planning.Action | None,
    state_repository: planning.lifted.StateRepository,
    axiom_evaluator: planning.lifted.AxiomEvaluator,
) -> None:
    if action is None:
        _check_callback_visits(
            generator.get_labeled_successor_nodes(node, state_repository, axiom_evaluator),
            lambda callback: generator.for_each_labeled_successor_node(
                node, state_repository, axiom_evaluator, callback
            ),
            lambda successor: (successor.label, successor.node),
        )
    else:
        _check_callback_visits(
            generator.get_labeled_successor_nodes(node, action, state_repository, axiom_evaluator),
            lambda callback: generator.for_each_labeled_successor_node(
                node, action, state_repository, axiom_evaluator, callback
            ),
            lambda successor: (successor.label, successor.node),
        )


def _check_successor_callbacks_ground(task: planning.ground.Task, *, labeled: bool) -> None:
    state_repository, axiom_evaluator, generator = _make_components_ground(task)
    node = generator.get_initial_node(state_repository, axiom_evaluator)
    check = _check_labeled_successor_callbacks_ground if labeled else _check_unlabeled_successor_callbacks_ground

    for action in [None, *task.get_task().get_domain().get_actions()]:
        check(generator, node, action, state_repository, axiom_evaluator)

    visit = generator.for_each_labeled_successor_node if labeled else generator.for_each_successor_node
    with pytest.raises(RuntimeError, match="callback failure"):
        visit(node, state_repository, axiom_evaluator, _fail_callback)
    assert visit(node, state_repository, axiom_evaluator, _continue_callback)


def _check_successor_callbacks_lifted(task: planning.lifted.Task, *, labeled: bool) -> None:
    state_repository, axiom_evaluator, generator = _make_components_lifted(task)
    node = generator.get_initial_node(state_repository, axiom_evaluator)
    check = _check_labeled_successor_callbacks_lifted if labeled else _check_unlabeled_successor_callbacks_lifted

    for action in [None, *task.get_task().get_domain().get_actions()]:
        check(generator, node, action, state_repository, axiom_evaluator)

    visit = generator.for_each_labeled_successor_node if labeled else generator.for_each_successor_node
    with pytest.raises(RuntimeError, match="callback failure"):
        visit(node, state_repository, axiom_evaluator, _fail_callback)
    assert visit(node, state_repository, axiom_evaluator, _continue_callback)


@pytest.mark.parametrize("family", ["ground", "lifted"])
@pytest.mark.parametrize("labeled", [False, True])
def test_successor_callbacks_match_lists_stop_and_propagate_exceptions(family: str, labeled: bool) -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    if family == "ground":
        _check_successor_callbacks_ground(ground_task, labeled=labeled)
    else:
        _check_successor_callbacks_lifted(lifted_task, labeled=labeled)


def _collect_successor_callback_values_ground(
    task: planning.ground.Task, *, labeled: bool
) -> tuple[list[object], list[str]]:
    state_repository, axiom_evaluator, generator = _make_components_ground(task)
    node = generator.get_initial_node(state_repository, axiom_evaluator)
    visit = generator.for_each_labeled_successor_node if labeled else generator.for_each_successor_node
    seen: list[object] = []

    def record(successor: object) -> bool:
        seen.append(successor)
        return True

    assert visit(node, state_repository, axiom_evaluator, record)
    return seen, [str(successor) for successor in seen]


def _collect_successor_callback_values_lifted(
    task: planning.lifted.Task, *, labeled: bool
) -> tuple[list[object], list[str]]:
    state_repository, axiom_evaluator, generator = _make_components_lifted(task)
    node = generator.get_initial_node(state_repository, axiom_evaluator)
    visit = generator.for_each_labeled_successor_node if labeled else generator.for_each_successor_node
    seen: list[object] = []

    def record(successor: object) -> bool:
        seen.append(successor)
        return True

    assert visit(node, state_repository, axiom_evaluator, record)
    return seen, [str(successor) for successor in seen]


@pytest.mark.parametrize("family", ["ground", "lifted"])
@pytest.mark.parametrize("labeled", [False, True])
def test_successor_callback_values_retain_their_repository(family: str, labeled: bool) -> None:
    def collect() -> tuple[list[object], list[str]]:
        ground_task, lifted_task = _make_gripper_tasks()
        if family == "ground":
            return _collect_successor_callback_values_ground(ground_task, labeled=labeled)
        return _collect_successor_callback_values_lifted(lifted_task, labeled=labeled)

    seen, expected = collect()
    gc.collect()
    assert seen
    assert [str(successor) for successor in seen] == expected


def _check_packed_plans_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, generator = _make_components_ground(task)
    node = generator.get_initial_node(state_repository, axiom_evaluator)
    first = generator.get_labeled_successor_nodes(node, state_repository, axiom_evaluator)[0]
    second = generator.get_labeled_successor_nodes(first.node, state_repository, axiom_evaluator)[0]
    step_lists: tuple[list[planning.ground.LabeledNode], ...] = ([], [first, second])
    for steps in step_lists:
        expected = [(step.label, step.node) for step in steps]
        plan = planning.ground.Plan(node, steps)
        for packed in (plan.pack(), planning.ground.PackedPlan(node.pack(), [step.pack() for step in steps])):
            assert type(packed) is planning.ground.PackedPlan
            assert packed.get_start_node().unpack() == node
            assert packed.get_length() == plan.get_length() == len(steps)
            assert packed.get_cost() == plan.get_cost()
            assert packed.empty() == plan.empty()
            assert [(step.label, step.node.unpack()) for step in packed.get_labeled_succ_nodes()] == expected
            unpacked = packed.unpack()
            assert type(unpacked) is planning.ground.Plan
            assert unpacked.get_start_node() == node
            assert unpacked.get_cost() == plan.get_cost()
            assert unpacked.get_length() == plan.get_length()
            assert [(step.label, step.node) for step in unpacked.get_labeled_succ_nodes()] == expected
            assert repr(packed) == str(packed)
    assert planning.ground.PackedPlan(node.pack()).empty()


def _check_packed_plans_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, generator = _make_components_lifted(task)
    node = generator.get_initial_node(state_repository, axiom_evaluator)
    first = generator.get_labeled_successor_nodes(node, state_repository, axiom_evaluator)[0]
    second = generator.get_labeled_successor_nodes(first.node, state_repository, axiom_evaluator)[0]
    step_lists: tuple[list[planning.lifted.LabeledNode], ...] = ([], [first, second])
    for steps in step_lists:
        expected = [(step.label, step.node) for step in steps]
        plan = planning.lifted.Plan(node, steps)
        for packed in (plan.pack(), planning.lifted.PackedPlan(node.pack(), [step.pack() for step in steps])):
            assert type(packed) is planning.lifted.PackedPlan
            assert packed.get_start_node().unpack() == node
            assert packed.get_length() == plan.get_length() == len(steps)
            assert packed.get_cost() == plan.get_cost()
            assert packed.empty() == plan.empty()
            assert [(step.label, step.node.unpack()) for step in packed.get_labeled_succ_nodes()] == expected
            unpacked = packed.unpack()
            assert type(unpacked) is planning.lifted.Plan
            assert unpacked.get_start_node() == node
            assert unpacked.get_cost() == plan.get_cost()
            assert unpacked.get_length() == plan.get_length()
            assert [(step.label, step.node) for step in unpacked.get_labeled_succ_nodes()] == expected
            assert repr(packed) == str(packed)
    assert planning.lifted.PackedPlan(node.pack()).empty()


def test_packed_plans_preserve_empty_and_multistep_plans() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_packed_plans_ground(ground_task)
    _check_packed_plans_lifted(lifted_task)


def _check_labeled_node_plan_construction_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_ground(task)
    start_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)
    labeled_successor_nodes = successor_generator.get_labeled_successor_nodes(
        start_node, state_repository, axiom_evaluator
    )

    assert labeled_successor_nodes
    first_labeled_successor = labeled_successor_nodes[0]
    assert type(first_labeled_successor.label) is formalism_planning.ActionBinding
    assert (
        successor_generator.get_successor_node(
            start_node,
            first_labeled_successor.label,
            state_repository,
            axiom_evaluator,
        )
        == first_labeled_successor.node
    )
    assert (
        successor_generator.get_successor_node(
            start_node,
            successor_generator.ground_action(first_labeled_successor.label),
            state_repository,
            axiom_evaluator,
        )
        == first_labeled_successor.node
    )

    labeled_node = planning.ground.LabeledNode(
        first_labeled_successor.label,
        first_labeled_successor.node,
    )
    plan = planning.ground.Plan(start_node, [labeled_node])

    assert plan.get_length() == 1
    assert not plan.empty()
    assert plan.get_cost() == labeled_node.node.get_metric()
    assert plan.get_start_node() == start_node
    assert plan.get_labeled_succ_nodes()[0].label == labeled_node.label
    assert plan.get_labeled_succ_nodes()[0].node == labeled_node.node


def _check_labeled_node_plan_construction_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = _make_components_lifted(task)
    start_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)
    labeled_successor_nodes = successor_generator.get_labeled_successor_nodes(
        start_node, state_repository, axiom_evaluator
    )

    assert labeled_successor_nodes
    first_labeled_successor = labeled_successor_nodes[0]
    assert type(first_labeled_successor.label) is formalism_planning.ActionBinding
    assert (
        successor_generator.get_successor_node(
            start_node,
            first_labeled_successor.label,
            state_repository,
            axiom_evaluator,
        )
        == first_labeled_successor.node
    )
    assert (
        successor_generator.get_successor_node(
            start_node,
            successor_generator.ground_action(first_labeled_successor.label),
            state_repository,
            axiom_evaluator,
        )
        == first_labeled_successor.node
    )

    labeled_node = planning.lifted.LabeledNode(
        first_labeled_successor.label,
        first_labeled_successor.node,
    )
    plan = planning.lifted.Plan(start_node, [labeled_node])

    assert plan.get_length() == 1
    assert not plan.empty()
    assert plan.get_cost() == labeled_node.node.get_metric()
    assert plan.get_start_node() == start_node
    assert plan.get_labeled_succ_nodes()[0].label == labeled_node.label
    assert plan.get_labeled_succ_nodes()[0].node == labeled_node.node


def test_labeled_node_is_constructible_for_plan_construction() -> None:
    ground_task, lifted_task = _make_gripper_tasks()
    _check_labeled_node_plan_construction_ground(ground_task)
    _check_labeled_node_plan_construction_lifted(lifted_task)


def test_state_views_from_independent_repository_factories_use_distinct_storage_identity() -> None:
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        execution_context = ExecutionContext(1)
        axiom_evaluator_factory = task_module.AxiomEvaluatorFactory()
        first_axiom_evaluator = axiom_evaluator_factory.create(task, execution_context)
        second_axiom_evaluator = axiom_evaluator_factory.create(task, execution_context)

        first_repository = task_module.StateRepositoryFactory().create(task)
        second_repository = task_module.StateRepositoryFactory().create(task)

        first_state = first_repository.get_initial_state(first_axiom_evaluator)
        second_state = second_repository.get_initial_state(second_axiom_evaluator)

        assert first_repository.get_worker_index() == 0
        assert second_repository.get_worker_index() == 0
        assert first_state.get_index() == second_state.get_index()
        assert first_state != second_state
        assert len({first_state, second_state}) == 2
        assert first_state.pack().get_index() == second_state.pack().get_index()
        assert first_state.pack() != second_state.pack()
        assert len({first_state.pack(), second_state.pack()}) == 2
