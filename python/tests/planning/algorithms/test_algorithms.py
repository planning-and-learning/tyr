# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
from datetime import timedelta
from typing import Self
import pytest
from pyyggdrasil.execution import ExecutionContext
from typing_extensions import override
from pytyr import formalism, planning
from python.tests.planning.support.components import algorithm_bindings, make_components_ground, make_components_lifted, make_gripper_tasks


def _check_lazy_gbfs_relaxed_plan_requests_ground(task: planning.ground.Task) -> None:
    relaxed_plans: tuple[set[formalism.RowActionLifted] | None, ...] = (set(), None)
    for plan in relaxed_plans:

        class RelaxedPlanHeuristic(planning.ground.Heuristic):
            def __init__(self, plan: set[formalism.RowActionLifted] | None) -> None:
                super().__init__()
                self.plan = plan
                self.num_plan_requests = 0

            @override
            def set_goal(self, goal: formalism.ConjunctiveConditionGround) -> None:
                pass

            @override
            def evaluate(self, state: planning.ground.StateBuilderView) -> float:
                return 0.0

            def make_worker(self, _execution_context: ExecutionContext) -> Self:
                return self

            @override
            def get_relaxed_plan(self) -> set[formalism.RowActionLifted] | None:
                self.num_plan_requests += 1
                return self.plan

        heuristic = RelaxedPlanHeuristic(plan)
        state_repository, axiom_evaluator, successor_generator = make_components_ground(task)
        result = planning.algorithms.gbfs_lazy.ground.find_solution(
            task,
            state_repository,
            axiom_evaluator,
            successor_generator,
            heuristic,
            planning.algorithms.gbfs_lazy.ground.Options(),
        )

        assert result.status == planning.SearchStatus.SOLVED
        assert heuristic.num_plan_requests == result.statistics.get_num_expanded()


def _check_lazy_gbfs_relaxed_plan_requests_lifted(task: planning.lifted.Task) -> None:
    relaxed_plans: tuple[set[formalism.RowActionLifted] | None, ...] = (set(), None)
    for plan in relaxed_plans:

        class RelaxedPlanHeuristic(planning.lifted.Heuristic):
            def __init__(self, plan: set[formalism.RowActionLifted] | None) -> None:
                super().__init__()
                self.plan = plan
                self.num_plan_requests = 0

            @override
            def set_goal(self, goal: formalism.ConjunctiveConditionGround) -> None:
                pass

            @override
            def evaluate(self, state: planning.lifted.StateBuilderView) -> float:
                return 0.0

            def make_worker(self, _execution_context: ExecutionContext) -> Self:
                return self

            @override
            def get_relaxed_plan(self) -> set[formalism.RowActionLifted] | None:
                self.num_plan_requests += 1
                return self.plan

        heuristic = RelaxedPlanHeuristic(plan)
        state_repository, axiom_evaluator, successor_generator = make_components_lifted(task)
        result = planning.algorithms.gbfs_lazy.lifted.find_solution(
            task,
            state_repository,
            axiom_evaluator,
            successor_generator,
            heuristic,
            planning.algorithms.gbfs_lazy.lifted.Options(),
        )

        assert result.status == planning.SearchStatus.SOLVED
        assert heuristic.num_plan_requests == result.statistics.get_num_expanded()


def test_planning_modules_export_expected_algorithm_submodules() -> None:
    for kind in ("ground", "lifted"):
        for algorithm_name in ("astar_eager", "brfs", "gbfs_lazy", "iw", "siw"):
            solver = getattr(getattr(planning.algorithms, algorithm_name), kind).Solver()
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

    progress_statistics = planning.ProgressStatistics()

    assert progress_statistics.get_snapshots() == []
    assert progress_statistics.empty()
    assert progress_statistics.size() == 0
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
        for algorithm_module in (
            algorithm_bindings(task_module, "astar_eager"),
            algorithm_bindings(task_module, "gbfs_lazy"),
        ):
            algorithm_module.DefaultEventHandler()

        for event_handler in (
            algorithm_bindings(task_module, "brfs").DefaultEventHandler(),
            algorithm_bindings(task_module, "brfs").DefaultEventHandler(0),
        ):
            assert isinstance(event_handler.get_progress_statistics(), planning.ProgressStatistics)

        for event_handler in (
            algorithm_bindings(task_module, "iw").DefaultEventHandler(),
            algorithm_bindings(task_module, "iw").DefaultEventHandler(0),
        ):
            assert isinstance(event_handler.get_statistics(), algorithm_bindings(task_module, "iw").Statistics)

        for event_handler in (
            algorithm_bindings(task_module, "siw").DefaultEventHandler(),
            algorithm_bindings(task_module, "siw").DefaultEventHandler(0),
        ):
            assert isinstance(event_handler.get_statistics(), algorithm_bindings(task_module, "siw").Statistics)


def test_default_event_handler_statistics_keep_handlers_alive() -> None:
    for task_module in (planning.ground, planning.lifted):
        event_handler = algorithm_bindings(task_module, "brfs").DefaultEventHandler()
        progress_statistics = event_handler.get_progress_statistics()

        del event_handler
        gc.collect()

        assert progress_statistics.empty()

        event_handler = algorithm_bindings(task_module, "iw").DefaultEventHandler()
        statistics = event_handler.get_statistics()

        del event_handler
        gc.collect()

        assert statistics.get_solution_arity() is None

        event_handler = algorithm_bindings(task_module, "siw").DefaultEventHandler()
        statistics = event_handler.get_statistics()

        del event_handler
        gc.collect()

        assert statistics.get_num_solved_subsearches() == 0


def test_algorithm_statistics_are_constructible_and_mutable() -> None:
    for task_module in (planning.ground, planning.lifted):
        iw_statistics = algorithm_bindings(task_module, "iw").Statistics()

        assert iw_statistics.get_solution_arity() is None
        assert repr(iw_statistics) == str(iw_statistics)

        iw_statistics.set_solution_arity(3)
        assert iw_statistics.get_solution_arity() == 3

        iw_statistics.clear()
        assert iw_statistics.get_solution_arity() is None

        siw_statistics = algorithm_bindings(task_module, "siw").Statistics()

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
        iw_solver = algorithm_bindings(task_module, "iw").Solver()

        assert isinstance(iw_solver.brfs_solver, algorithm_bindings(task_module, "brfs").Solver)
        assert iw_solver.brfs_solver.task is None
        assert iw_solver.brfs_solver.state_repository is None
        assert iw_solver.brfs_solver.axiom_evaluator is None
        assert iw_solver.brfs_solver.successor_generator is None
        assert iw_solver.max_arity == iw_max_arity
        assert iw_solver.options is not None

        siw_solver = algorithm_bindings(task_module, "siw").Solver()

        assert isinstance(siw_solver.iw_solver, algorithm_bindings(task_module, "iw").Solver)
        assert isinstance(siw_solver.iw_solver.brfs_solver, algorithm_bindings(task_module, "brfs").Solver)
        assert siw_solver.iw_solver.brfs_solver.task is None
        assert siw_solver.iw_solver.brfs_solver.state_repository is None
        assert siw_solver.iw_solver.brfs_solver.axiom_evaluator is None
        assert siw_solver.iw_solver.brfs_solver.successor_generator is None
        assert siw_solver.iw_solver.max_arity == iw_max_arity
        assert siw_solver.options is not None


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
        for algorithm in (
            algorithm_bindings(task_module, "brfs"),
            algorithm_bindings(task_module, "astar_eager"),
            algorithm_bindings(task_module, "gbfs_lazy"),
            algorithm_bindings(task_module, "iw"),
        ):
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

    for task_module in (planning.ground, planning.lifted):
        for algorithm_name, expected_defaults in expected_defaults_by_algorithm.items():
            options = algorithm_bindings(task_module, algorithm_name).Options()

            for field, expected_default in expected_defaults.items():
                assert getattr(options, field) == expected_default

            if algorithm_name != "siw":
                assert isinstance(options.search_budget, planning.SearchBudget)
                assert options.search_budget.max_num_states is None
                assert options.search_budget.max_time is None


def test_lazy_gbfs_reads_the_relaxed_plan_of_python_heuristics() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_lazy_gbfs_relaxed_plan_requests_ground(ground_task)
    _check_lazy_gbfs_relaxed_plan_requests_lifted(lifted_task)


def test_cost_mode_is_bound_for_cost_sensitive_algorithms() -> None:
    assert planning.CostMode.UNIT != planning.CostMode.GENERAL

    for task_module in (planning.ground, planning.lifted):
        for algorithm_module in (
            algorithm_bindings(task_module, "astar_eager"),
            algorithm_bindings(task_module, "gbfs_lazy"),
        ):
            options = algorithm_module.Options()

            assert options.cost_mode == planning.CostMode.GENERAL
            options.cost_mode = planning.CostMode.UNIT
            assert options.cost_mode == planning.CostMode.UNIT
