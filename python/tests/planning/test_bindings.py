import gc
from datetime import timedelta
from types import ModuleType

import pytest

from pytyr import planning
from pypddl.formalism import ParserOptions
from pytyr.formalism import planning as formalism_planning
from pytyr.formalism.planning import Parser
from pyyggdrasil.execution import ExecutionContext

from pypddl_datasets import fetch_task

Task = planning.lifted.Task | planning.ground.Task

GRIPPER = fetch_task("classical/tests/gripper/test-1.pddl")


def test_planning_modules_export_expected_algorithm_submodules():
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


def test_planning_statistics_bindings_expose_counters_and_progress_snapshots():
    statistics = planning.Statistics()

    assert statistics.get_num_accepted_successors() == 0
    assert statistics.get_num_expanded() == 0
    assert statistics.get_num_deadends() == 0
    assert statistics.get_num_pruned() == 0
    assert statistics.get_num_generated_successors() == 0
    assert statistics.get_num_transferred_successors() == 0
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
    statistics.increment_num_accepted_successors()
    statistics.increment_num_expanded()
    statistics.increment_num_deadends()
    statistics.increment_num_pruned()
    statistics.increment_num_generated_successors(False)
    statistics.increment_num_generated_successors(True)
    assert statistics.get_num_accepted_successors() == 1
    assert statistics.get_num_expanded() == 1
    assert statistics.get_num_deadends() == 1
    assert statistics.get_num_pruned() == 1
    assert statistics.get_num_generated_successors() == 2
    assert statistics.get_num_transferred_successors() == 1
    assert statistics.get_communication_overhead() == 0.5
    statistics.clear()
    assert statistics.get_num_accepted_successors() == 0
    assert statistics.get_num_expanded() == 0
    assert statistics.get_num_deadends() == 0
    assert statistics.get_num_pruned() == 0
    assert statistics.get_num_generated_successors() == 0
    assert statistics.get_num_transferred_successors() == 0
    assert statistics.get_communication_overhead() == 0
    assert repr(statistics) == str(statistics)
    for label in (
        "Number of registered states",
        "State storage memory usage",
        "Action bindings memory usage",
        "Predicate bindings memory usage",
        "Axiom bindings memory usage",
        "Function bindings memory usage",
        "Number of generated successors",
        "Number of transferred successors",
        "Communication overhead",
        "Destination lock acquisitions",
        "Destination lock wait time",
        "Destination lock hold time",
    ):
        assert label in str(statistics)

    snapshot = planning.ProgressStatisticsSnapshot(1, 2, 3, 4)

    assert snapshot.get_num_accepted_successors() == 1
    assert snapshot.get_num_expanded() == 2
    assert snapshot.get_num_deadends() == 3
    assert snapshot.get_num_pruned() == 4
    assert repr(snapshot) == str(snapshot)
    assert "deadend" in str(snapshot)

    progress_statistics = planning.ProgressStatistics()

    assert progress_statistics.get_snapshots() == []
    assert progress_statistics.empty()
    assert progress_statistics.size() == 0
    progress_statistics.add_snapshot(statistics)
    progress_statistics.add_snap_shot(statistics)
    assert len(progress_statistics.get_snapshots()) == 2
    assert progress_statistics.size() == 2
    assert not progress_statistics.empty()
    assert progress_statistics.get_snapshots()[0].get_num_accepted_successors() == 0
    assert progress_statistics.get_snapshots()[1].get_num_accepted_successors() == 0
    progress_statistics.clear()
    assert progress_statistics.get_snapshots() == []
    assert progress_statistics.empty()
    assert progress_statistics.size() == 0

    progress_statistics = planning.ProgressStatistics()
    progress_statistics.add_snapshot(statistics)
    assert len(progress_statistics.get_snapshots()) == 1
    assert progress_statistics.get_snapshots()[0].get_num_accepted_successors() == 0
    assert repr(progress_statistics) == str(progress_statistics)
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


def test_default_event_handler_statistics_keep_handlers_alive():
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


def test_algorithm_statistics_are_constructible_and_mutable():
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


def test_nested_width_search_solvers_expose_expected_defaults():
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


def test_heuristics_expose_preferred_actions():
    for task_module in (planning.ground, planning.lifted):
        heuristic = task_module.BlindHeuristic()

        preferred_actions = heuristic.get_preferred_actions()

        del heuristic
        gc.collect()

        assert list(preferred_actions) == []


def test_search_result_exposes_all_result_fields():
    for task_module in (planning.ground, planning.lifted):
        result = task_module.SearchResult()

        assert result.status == planning.SearchStatus.IN_PROGRESS
        assert result.plan is None
        assert result.goal_node is None
        assert result.cycle_range is None
        assert isinstance(result.statistics, planning.Statistics)
        assert result.worker_statistics == []
        assert result.statistics.get_num_accepted_successors() == 0

        result.status = planning.SearchStatus.CYCLE
        result.cycle_range = (1, 3)
        result.statistics.increment_num_accepted_successors()

        assert result.status == planning.SearchStatus.CYCLE
        assert result.cycle_range == (1, 3)
        assert result.statistics.get_num_accepted_successors() == 1


def _assert_worker_statistics(result, num_workers: int):
    assert len(result.worker_statistics) == num_workers
    for getter in (
        "get_num_accepted_successors",
        "get_num_expanded",
        "get_num_deadends",
        "get_num_pruned",
        "get_num_generated_successors",
        "get_num_transferred_successors",
        "get_num_registered_states",
        "get_state_storage_memory_usage",
    ):
        worker_total = sum(getattr(worker, getter)() for worker in result.worker_statistics)
        assert worker_total == getattr(result.statistics, getter)()
    assert result.statistics.get_communication_overhead() == (
        result.statistics.get_num_transferred_successors() / result.statistics.get_num_generated_successors()
        if result.statistics.get_num_generated_successors()
        else 0
    )
    worker_idle_time = sum(
        (worker.get_idle_time() for worker in result.worker_statistics),
        start=timedelta(0),
    )
    assert abs(worker_idle_time - result.statistics.get_idle_time()) <= timedelta(
        microseconds=num_workers
    )
    assert sum(worker.get_num_destination_lock_acquisitions() for worker in result.worker_statistics) == (
        result.statistics.get_num_destination_lock_acquisitions()
    )
    worker_lock_wait_time = sum(
        (worker.get_destination_lock_wait_time() for worker in result.worker_statistics),
        start=timedelta(0),
    )
    assert abs(worker_lock_wait_time - result.statistics.get_destination_lock_wait_time()) <= timedelta(microseconds=num_workers)
    worker_lock_hold_time = sum(
        (worker.get_destination_lock_hold_time() for worker in result.worker_statistics),
        start=timedelta(0),
    )
    assert abs(worker_lock_hold_time - result.statistics.get_destination_lock_hold_time()) <= timedelta(microseconds=num_workers)


def test_algorithm_options_are_default_constructible_with_expected_fields():
    ygg_uint_max = (1 << 32) - 1
    expected_defaults_by_algorithm = {
        "astar_eager": {
            "start_node": None,
            "event_handler": None,
            "pruning_strategy": None,
            "goal_strategy": None,
            "max_num_states": ygg_uint_max,
            "max_time": None,
            "cost_mode": planning.CostMode.GENERAL,
            "num_search_workers": 1,
            "dist_hash_mode": planning.DistHashMode.LMCUT,
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
            "max_num_states": ygg_uint_max,
            "max_time": None,
            "num_search_workers": 1,
            "dist_hash_mode": planning.DistHashMode.LMCUT,
            "collect_destination_lock_statistics": False,
            "random_seed": 0,
            "shuffle_labeled_succ_nodes": False,
        },
        "gbfs_lazy": {
            "start_node": None,
            "event_handler": None,
            "pruning_strategy": None,
            "goal_strategy": None,
            "max_num_states": ygg_uint_max,
            "max_time": None,
            "cost_mode": planning.CostMode.GENERAL,
            "use_preferred_actions": True,
            "boost_preferred_queue": 1000,
            "num_search_workers": 1,
            "dist_hash_mode": planning.DistHashMode.LMCUT,
            "collect_destination_lock_statistics": False,
            "random_seed": 0,
            "shuffle_labeled_succ_nodes": False,
        },
        "iw": {
            "start_node": None,
            "event_handler": None,
            "goal_strategy": None,
            "max_num_states": ygg_uint_max,
            "max_time": None,
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
            "max_num_states",
            "max_time",
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
            "max_num_states",
            "max_time",
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
            "max_num_states",
            "max_time",
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
            "max_num_states",
            "max_time",
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


def _make_gripper_tasks():
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


def _make_components(task_module: ModuleType, task: Task, *, concurrent: bool = False):
    execution_context = ExecutionContext(1)
    axiom_evaluator = task_module.AxiomEvaluatorFactory().create(task, execution_context)
    state_repository_factory = task_module.StateRepositoryFactory()
    state_repository = (
        state_repository_factory.create_concurrent(task)
        if concurrent
        else state_repository_factory.create(task)
    )
    successor_generator = task_module.SuccessorGeneratorFactory().create(task, execution_context)
    return state_repository, axiom_evaluator, successor_generator


def test_parallel_search_is_available_through_python_bindings():
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        class EventHandler(task_module.gbfs_lazy.EventHandler):
            def __init__(self):
                super().__init__()
                self.end_status = None
                self.num_expanded = None
                self.solved = False

            def on_start_search(self, node, h_value):
                pass

            def on_new_best_h_value(self, h_value):
                pass

            def on_end_search(self, status, statistics):
                self.end_status = status
                self.num_expanded = statistics.get_num_expanded()

            def on_solved(self, plan):
                self.solved = True

        state_repository, axiom_evaluator, successor_generator = _make_components(task_module, task, concurrent=True)
        event_handler = EventHandler()
        options = task_module.gbfs_lazy.Options()
        options.event_handler = event_handler
        options.num_search_workers = 2
        options.collect_destination_lock_statistics = True

        result = task_module.gbfs_lazy.find_solution(
            task,
            state_repository,
            axiom_evaluator,
            successor_generator,
            task_module.BlindHeuristic(),
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

        brfs_options = task_module.brfs.Options()
        brfs_options.num_search_workers = 2
        brfs_result = task_module.brfs.find_solution(
            task,
            state_repository,
            axiom_evaluator,
            successor_generator,
            brfs_options,
        )
        assert brfs_result.status == planning.SearchStatus.SOLVED
        assert brfs_result.goal_node.get_state().get_state_repository() == state_repository
        _assert_worker_statistics(brfs_result, 2)

        astar_options = task_module.astar_eager.Options()
        astar_options.num_search_workers = 2
        for mode in (planning.ParallelSearchMode.SYNCHRONOUS, planning.ParallelSearchMode.ASYNCHRONOUS):
            astar_options.parallel_search_mode = mode
            astar_result = task_module.astar_eager.find_solution(
                task,
                state_repository,
                axiom_evaluator,
                successor_generator,
                task_module.BlindHeuristic(),
                astar_options,
            )
            assert astar_result.status == planning.SearchStatus.SOLVED
            _assert_worker_statistics(astar_result, 2)


def test_parallel_search_rejects_null_python_heuristic_workers():
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        class NullWorkerHeuristic(task_module.Heuristic):
            def __init__(self):
                super().__init__()

            def set_goal(self, goal):
                pass

            def evaluate(self, state):
                return 0.0

            def make_worker(self, execution_context):
                return None

        state_repository, axiom_evaluator, successor_generator = _make_components(task_module, task)
        options = task_module.astar_eager.Options()
        options.num_search_workers = 2

        with pytest.raises(ValueError, match="heuristic does not support worker construction"):
            task_module.astar_eager.find_solution(
                task,
                state_repository,
                axiom_evaluator,
                successor_generator,
                NullWorkerHeuristic(),
                options,
            )


def test_parallel_search_constructs_python_goal_and_pruning_workers():
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        state_repository, axiom_evaluator, successor_generator = _make_components(task_module, task, concurrent=True)
        initial_state = state_repository.get_initial_state(axiom_evaluator)
        fluent_variables = [fact.get_variable() for fact in initial_state.fluent_facts()]
        factory_indices = {"goal": [], "pruning": []}
        worker_ids = {"goal": set(), "pruning": set()}
        callback_indices = {"goal": [], "pruning": []}
        retained_snapshots = []

        class PythonGoalStrategy(task_module.GoalStrategy):
            def __init__(self, worker_index=None):
                super().__init__()
                self.worker_index = worker_index

            def make_worker(self, worker_index):
                factory_indices["goal"].append((isinstance(worker_index, planning.WorkerIndex), int(worker_index)))
                worker = PythonGoalStrategy(int(worker_index))
                worker_ids["goal"].add(id(worker))
                return worker

            def is_static_goal_satisfied(self, task):
                callback_indices["goal"].append(self.worker_index)
                return True

            def is_dynamic_goal_satisfied(self, seed_state, state):
                callback_indices["goal"].append(self.worker_index)
                if len(callback_indices["goal"]) > 2 and not retained_snapshots:
                    retained_snapshots.append((state, tuple(state.get(variable) for variable in fluent_variables)))
                return False

        class PythonPruningStrategy(task_module.PruningStrategy):
            def __init__(self, worker_index=None):
                super().__init__()
                self.worker_index = worker_index

            def make_worker(self, worker_index):
                factory_indices["pruning"].append((isinstance(worker_index, planning.WorkerIndex), int(worker_index)))
                worker = PythonPruningStrategy(int(worker_index))
                worker_ids["pruning"].add(id(worker))
                return worker

            def should_prune_state(self, state):
                callback_indices["pruning"].append(self.worker_index)
                return False

            def should_prune_successor_state(self, state, succ_state, is_new_succ_state):
                callback_indices["pruning"].append(self.worker_index)
                return False

        options = task_module.brfs.Options()
        options.num_search_workers = 2
        options.goal_strategy = PythonGoalStrategy()
        options.pruning_strategy = PythonPruningStrategy()

        result = task_module.brfs.find_solution(task, state_repository, axiom_evaluator, successor_generator, options)

        assert result.status == planning.SearchStatus.EXHAUSTED
        for strategy in ("goal", "pruning"):
            assert factory_indices[strategy] == [(True, 1)]
            assert len(worker_ids[strategy]) == 1
            assert set(callback_indices[strategy]) == {None, 1}

        assert "Owning read-only state snapshot" in task_module.StateBuilder.__doc__
        assert retained_snapshots
        snapshot, values = retained_snapshots[0]
        gc.collect()
        assert tuple(snapshot.get(variable) for variable in fluent_variables) == values


def test_parallel_search_rejects_missing_or_null_python_strategy_workers():
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        class MissingWorkerGoalStrategy(task_module.GoalStrategy):
            def is_static_goal_satisfied(self, task):
                return True

            def is_dynamic_goal_satisfied(self, seed_state, state):
                return False

        class NullWorkerGoalStrategy(MissingWorkerGoalStrategy):
            def make_worker(self, worker_index):
                return None

        class MissingWorkerPruningStrategy(task_module.PruningStrategy):
            pass

        class NullWorkerPruningStrategy(MissingWorkerPruningStrategy):
            def make_worker(self, worker_index):
                return None

        state_repository, axiom_evaluator, successor_generator = _make_components(task_module, task)
        cases = (
            ("goal_strategy", MissingWorkerGoalStrategy(), "goal strategy does not support worker construction"),
            ("goal_strategy", NullWorkerGoalStrategy(), "goal strategy does not support worker construction"),
            ("pruning_strategy", MissingWorkerPruningStrategy(), "pruning strategy does not support worker construction"),
            ("pruning_strategy", NullWorkerPruningStrategy(), "pruning strategy does not support worker construction"),
        )

        for field, strategy, error in cases:
            options = task_module.brfs.Options()
            options.num_search_workers = 2
            setattr(options, field, strategy)
            with pytest.raises(ValueError, match=error):
                task_module.brfs.find_solution(task, state_repository, axiom_evaluator, successor_generator, options)


def test_planning_task_view_accessors_keep_temporary_owners_alive():
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
    ground_task_view = planning.lifted.Task(formalism_task).instantiate_ground_task(
        ExecutionContext(1),
        planning.lifted.GroundTaskInstantiationOptions(),
    ).task.get_task()

    gc.collect()

    assert ground_task_view.get_name() == "gripper-2"
    assert [object_.get_name() for object_ in ground_task_view.get_objects()] == ["ball1", "ball2", "left", "right"]


def test_cost_mode_is_bound_for_cost_sensitive_algorithms():
    assert planning.CostMode.UNIT != planning.CostMode.GENERAL

    for task_module in (planning.ground, planning.lifted):
        for algorithm_module in (task_module.astar_eager, task_module.gbfs_lazy):
            options = algorithm_module.Options()

            assert options.cost_mode == planning.CostMode.GENERAL
            options.cost_mode = planning.CostMode.UNIT
            assert options.cost_mode == planning.CostMode.UNIT


def test_parallel_astar_search_mode_is_bound():
    assert planning.ParallelSearchMode.SYNCHRONOUS != planning.ParallelSearchMode.ASYNCHRONOUS

    for task_module in (planning.ground, planning.lifted):
        options = task_module.astar_eager.Options()
        assert options.parallel_search_mode == planning.ParallelSearchMode.SYNCHRONOUS
        options.parallel_search_mode = planning.ParallelSearchMode.ASYNCHRONOUS
        assert options.parallel_search_mode == planning.ParallelSearchMode.ASYNCHRONOUS


def test_state_repository_concurrency_is_bound():
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        factory = task_module.StateRepositoryFactory()
        assert not factory.create(task).is_concurrent()
        assert factory.create_concurrent(task).is_concurrent()


def test_dist_hash_mode_is_bound():
    assert planning.DistHashMode.RANDOM != planning.DistHashMode.LMCUT

    for task_module in (planning.ground, planning.lifted):
        for algorithm_module in (
            task_module.astar_eager,
            task_module.brfs,
            task_module.gbfs_lazy,
        ):
            options = algorithm_module.Options()
            assert options.dist_hash_mode == planning.DistHashMode.LMCUT
            options.dist_hash_mode = planning.DistHashMode.RANDOM
            assert options.dist_hash_mode == planning.DistHashMode.RANDOM


def test_ground_task_instantiation_result_default_is_explicit_failure():
    result = planning.lifted.GroundTaskInstantiationResult()

    assert result.task is None
    assert result.status == planning.lifted.GroundTaskInstantiationStatus.PROVEN_UNSOLVABLE


def test_lifted_task_instantiates_ground_task_from_parsed_pddl():
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

    assert result.status == planning.lifted.GroundTaskInstantiationStatus.SUCCESS
    assert isinstance(result.task, planning.ground.Task)
    assert result.task.get_task().get_name() == "gripper-2"


def test_state_repository_create_state_uses_plural_fluent_facts_argument():
    docstring = planning.ground.StateRepository.create_state.__doc__ or ""

    assert "fluent_facts" in docstring
    assert "fluent_fact:" not in docstring


def test_state_get_uses_fluent_variable_argument_name():
    docstring = planning.ground.State.get.__doc__ or ""

    assert "fluent_variable" in docstring
    assert "fluent_fact:" not in docstring


def test_state_iterable_methods_return_stable_iterators():
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        state_repository, axiom_evaluator, _ = _make_components(task_module, task)
        state = state_repository.get_initial_state(axiom_evaluator)

        static_atoms = state.static_atoms()

        assert iter(static_atoms) is static_atoms
        first_static_atom = next(static_atoms)
        assert first_static_atom in list(state.static_atoms())
        assert len(list(static_atoms)) == 11
        assert list(static_atoms) == []
        assert len(list(state.fluent_facts())) == 5
        assert len(list(state.derived_atoms())) == 0
        assert len(list(state.static_fterm_values())) == 0
        assert len(list(state.fluent_fterm_values())) == 0

        static_atoms = state.static_atoms()
        del state
        gc.collect()
        assert len(list(static_atoms)) == 12


def test_state_repository_create_state_accepts_state_iterables():
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        state_repository, axiom_evaluator, _ = _make_components(task_module, task)

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


def test_planning_state_node_and_plan_repr_matches_str():
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        state_repository, axiom_evaluator, successor_generator = _make_components(task_module, task)
        state = state_repository.get_initial_state(axiom_evaluator)
        node = task_module.Node(state, 0.0)
        labeled_node = successor_generator.get_labeled_successor_nodes(node, state_repository, axiom_evaluator)[0]
        plan = task_module.Plan(node)

        assert repr(state) == str(state)
        assert repr(node) == str(node)
        assert repr(labeled_node) == str(labeled_node)
        assert repr(plan) == str(plan)
        assert plan.get_length() == 0
        assert plan.empty()


def test_node_python_equality_and_hash_follow_state_and_metric():
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        state_repository, axiom_evaluator, _ = _make_components(task_module, task)
        state = state_repository.get_initial_state(axiom_evaluator)

        first_node = task_module.Node(state, 1.0)
        second_node = task_module.Node(state, 1.0)
        different_metric_node = task_module.Node(state, 2.0)

        assert first_node == second_node
        assert hash(first_node) == hash(second_node)
        assert first_node != different_metric_node


def test_successor_generator_accepts_explicit_repository_for_node_lookup():
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        state_repository, axiom_evaluator, successor_generator = _make_components(task_module, task)
        initial_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)
        looked_up_node = successor_generator.get_node(state_repository, initial_node.get_state().get_index())

        assert successor_generator.get_index() == 0
        assert not hasattr(successor_generator, "get_state_repository")
        assert looked_up_node == initial_node


def test_successor_generators_expose_uniform_action_binding_api():
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        state_repository, axiom_evaluator, successor_generator = _make_components(task_module, task)
        start_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)

        bindings = successor_generator.get_applicable_action_bindings(start_node)
        labeled_successor_nodes = successor_generator.get_labeled_successor_nodes(start_node, state_repository, axiom_evaluator)

        assert len(bindings) == len(labeled_successor_nodes)
        assert bindings

        binding = bindings[0]
        ground_action = successor_generator.ground_action(binding)
        binding_successor = successor_generator.get_successor_node(start_node, binding, state_repository, axiom_evaluator)
        ground_action_successor = successor_generator.get_successor_node(start_node, ground_action, state_repository, axiom_evaluator)

        assert isinstance(binding, formalism_planning.ActionBinding)
        assert successor_generator.ground_action(binding) == ground_action
        assert ground_action.get_action().get_arity() == len(list(ground_action.get_objects()))
        assert binding_successor == ground_action_successor
        assert any(
            labeled_successor.label == binding and labeled_successor.node == binding_successor
            for labeled_successor in labeled_successor_nodes
        )


def test_labeled_node_is_constructible_for_plan_construction():
    ground_task, lifted_task = _make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        state_repository, axiom_evaluator, successor_generator = _make_components(task_module, task)
        start_node = successor_generator.get_initial_node(state_repository, axiom_evaluator)
        labeled_successor_nodes = successor_generator.get_labeled_successor_nodes(start_node, state_repository, axiom_evaluator)

        assert labeled_successor_nodes
        first_labeled_successor = labeled_successor_nodes[0]
        assert isinstance(first_labeled_successor.label, formalism_planning.ActionBinding)
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

        labeled_node = task_module.LabeledNode(
            first_labeled_successor.label,
            first_labeled_successor.node,
        )
        plan = task_module.Plan(start_node, [labeled_node])

        assert plan.get_length() == 1
        assert not plan.empty()
        assert plan.get_cost() == labeled_node.node.get_metric()
        assert plan.get_start_node() == start_node
        assert plan.get_labeled_succ_nodes()[0].label == labeled_node.label
        assert plan.get_labeled_succ_nodes()[0].node == labeled_node.node


def test_state_views_from_independent_repository_factories_use_distinct_storage_identity():
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

        assert first_repository.get_index() == 0
        assert second_repository.get_index() == 0
        assert first_state.get_index() == second_state.get_index()
        assert first_state != second_state
        assert len({first_state, second_state}) == 2
