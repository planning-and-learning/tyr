# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
from pyyggdrasil.execution import ExecutionContext
from pytyr import formalism, planning
from python.tests.planning.support.components import make_components_ground, make_components_lifted, make_gripper_tasks


def _check_state_reads_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, _ = make_components_ground(task)
    state = state_repository.get_initial_state(axiom_evaluator)

    static_atoms = state.static_atoms()

    assert len(static_atoms) == 12
    assert static_atoms == state.static_atoms()
    assert len(state.fluent_facts()) == 5
    fluent_atoms = state.fluent_atoms()
    assert len(fluent_atoms) == 5
    assert all(state.test(atom) for atom in fluent_atoms)
    assert len(state.derived_atoms()) == 0
    assert len(state.static_fterm_values()) == 0
    assert len(state.fluent_fterm_values()) == 0

    static_atoms = state.static_atoms()
    del state
    gc.collect()
    assert len(static_atoms) == 12


def _check_state_reads_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, _ = make_components_lifted(task)
    state = state_repository.get_initial_state(axiom_evaluator)

    static_atoms = state.static_atoms()

    assert len(static_atoms) == 12
    assert static_atoms == state.static_atoms()
    assert len(state.fluent_facts()) == 5
    fluent_atoms = state.fluent_atoms()
    assert len(fluent_atoms) == 5
    assert all(state.test(atom) for atom in fluent_atoms)
    assert len(state.derived_atoms()) == 0
    assert len(state.static_fterm_values()) == 0
    assert len(state.fluent_fterm_values()) == 0

    static_atoms = state.static_atoms()
    del state
    gc.collect()
    assert len(static_atoms) == 12


def _check_packed_state_roundtrip_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_ground(task)
    state = state_repository.get_initial_state(axiom_evaluator)
    state_index = state.pack().index()
    state_text = str(state)
    packed = state.pack()

    assert type(packed) is planning.ground.State
    assert packed.index().value == state_index.value
    assert packed.context() == state_repository
    assert packed.unpack() == state
    assert packed == state.pack()
    assert hash(packed) == hash(state.pack())

    del state, state_repository, axiom_evaluator, successor_generator
    gc.collect()

    unpacked = packed.unpack()
    assert type(unpacked) is planning.ground.SharedStateBuilderView
    assert unpacked.pack().index() == state_index
    assert str(unpacked) == state_text
    assert unpacked.pack() == packed

    del packed
    gc.collect()
    assert str(unpacked) == state_text


def _check_packed_state_roundtrip_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task)
    state = state_repository.get_initial_state(axiom_evaluator)
    state_index = state.pack().index()
    state_text = str(state)
    packed = state.pack()

    assert type(packed) is planning.lifted.State
    assert packed.index().value == state_index.value
    assert packed.context() == state_repository
    assert packed.unpack() == state
    assert packed == state.pack()
    assert hash(packed) == hash(state.pack())

    del state, state_repository, axiom_evaluator, successor_generator
    gc.collect()

    unpacked = packed.unpack()
    assert type(unpacked) is planning.lifted.SharedStateBuilderView
    assert unpacked.pack().index() == state_index
    assert str(unpacked) == state_text
    assert unpacked.pack() == packed

    del packed
    gc.collect()
    assert str(unpacked) == state_text


def _check_create_state_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, _ = make_components_ground(task)

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
            formalism.FDRFactFluentData(fact.variable().index(), fact.value())
            for fact in initial_state.fluent_facts()
        ],
        [(fterm.index(), value) for fterm, value in initial_state.fluent_fterm_values()],
    )

    registered_state = state_repository.get_registered_state(initial_state.pack().index())

    assert registered_state == initial_state
    assert registered_state.pack().index() == initial_state.pack().index()
    assert recreated_state == initial_state
    assert recreated_state.pack().index() == initial_state.pack().index()
    assert value_recreated_state == initial_state
    assert value_recreated_state.pack().index() == initial_state.pack().index()
    assert state_repository.num_states() == 1


def _check_create_state_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, _ = make_components_lifted(task)

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
            formalism.FDRFactFluentData(fact.variable().index(), fact.value())
            for fact in initial_state.fluent_facts()
        ],
        [(fterm.index(), value) for fterm, value in initial_state.fluent_fterm_values()],
    )

    registered_state = state_repository.get_registered_state(initial_state.pack().index())

    assert registered_state == initial_state
    assert registered_state.pack().index() == initial_state.pack().index()
    assert recreated_state == initial_state
    assert recreated_state.pack().index() == initial_state.pack().index()
    assert value_recreated_state == initial_state
    assert value_recreated_state.pack().index() == initial_state.pack().index()
    assert state_repository.num_states() == 1


def test_state_repository_concurrency_is_bound() -> None:
    ground_task, lifted_task = make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        assert not task_module.StateRepository(task).is_concurrent()
        assert task_module.StateRepository(task, concurrent=True).is_concurrent()


def test_state_repository_create_state_uses_plural_fluent_facts_argument() -> None:
    docstring = planning.ground.StateRepository.create_state.__doc__ or ""

    assert "fluent_facts" in docstring
    assert "fluent_fact:" not in docstring


def test_state_get_names_its_parameter() -> None:
    docstring = planning.ground.SharedStateBuilderView.get.__doc__ or ""

    assert "view" in docstring
    assert "index" in docstring


def test_state_reads_return_lists() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_state_reads_ground(ground_task)
    _check_state_reads_lifted(lifted_task)


def test_packed_state_roundtrip_retains_repository() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_packed_state_roundtrip_ground(ground_task)
    _check_packed_state_roundtrip_lifted(lifted_task)


def test_state_repository_create_state_accepts_state_iterables() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_create_state_ground(ground_task)
    _check_create_state_lifted(lifted_task)


def test_state_views_from_independent_repository_factories_use_distinct_storage_identity() -> None:
    ground_task, lifted_task = make_gripper_tasks()

    for task_module, task in (
        (planning.ground, ground_task),
        (planning.lifted, lifted_task),
    ):
        execution_context = ExecutionContext(1)
        first_axiom_evaluator = task_module.AxiomEvaluator(task, execution_context)
        second_axiom_evaluator = task_module.AxiomEvaluator(task, execution_context)

        first_repository = task_module.StateRepository(task)
        second_repository = task_module.StateRepository(task)

        first_state = first_repository.get_initial_state(first_axiom_evaluator)
        second_state = second_repository.get_initial_state(second_axiom_evaluator)

        assert first_repository.get_worker_index() == 0
        assert second_repository.get_worker_index() == 0
        assert first_state.pack().index() == second_state.pack().index()
        assert first_state != second_state
        assert len({first_state, second_state}) == 2
        assert first_state.pack().index() == second_state.pack().index()
        assert first_state.pack() != second_state.pack()
        assert len({first_state.pack(), second_state.pack()}) == 2
