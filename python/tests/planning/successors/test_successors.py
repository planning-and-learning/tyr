# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
from collections import Counter
from collections.abc import Callable, Sequence
from typing import Never, TypeVar
import pytest
from pytyr import formalism, planning
from python.tests.planning.support.components import GroundNode, GroundSuccessors, LiftedNode, LiftedSuccessors, make_components_ground, make_components_lifted, make_gripper_tasks


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


def _check_node_lookup_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_ground(task)
    initial_node = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node()
    looked_up_node = planning.ground.get_node(state_repository, initial_node.state().pack().index())

    assert looked_up_node == initial_node
    assert (
        planning.ground.get_node(
            state_repository,
            GroundSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node().pack(),
        )
        == initial_node
    )
    assert (
        planning.ground.get_node(
            state_repository, planning.ground.get_node(state_repository, initial_node.state().pack().index()).pack()
        )
        == initial_node
    )


def _check_node_lookup_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task)
    initial_node = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node()
    looked_up_node = planning.lifted.get_node(state_repository, initial_node.state().pack().index())

    assert looked_up_node == initial_node
    assert (
        planning.lifted.get_node(
            state_repository,
            LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node().pack(),
        )
        == initial_node
    )
    assert (
        planning.lifted.get_node(
            state_repository, planning.lifted.get_node(state_repository, initial_node.state().pack().index()).pack()
        )
        == initial_node
    )


def _check_action_binding_api_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_ground(task)
    start_node = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node()

    bindings = GroundSuccessors.applicable_action_bindings(successor_generator, start_node)
    labeled_successor_nodes = GroundSuccessors(
        successor_generator, state_repository, axiom_evaluator
    ).labeled_successor_nodes(start_node)

    assert len(bindings) == len(labeled_successor_nodes)
    assert bindings

    binding = bindings[0]
    ground_action = successor_generator.ground_action(binding)
    binding_successor = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).successor_node(
        start_node, binding
    )
    ground_action_successor = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).successor_node(
        start_node, ground_action
    )

    assert type(binding) is formalism.RowActionLifted
    assert successor_generator.ground_action(binding) == ground_action
    assert ground_action.action().arity() == len(list(ground_action.row().resolve_objects()))
    assert binding_successor == ground_action_successor
    packed_binding_successor = (
        GroundSuccessors(successor_generator, state_repository, axiom_evaluator)
        .successor_node(start_node, binding)
        .pack()
    )
    packed_action_successor = (
        GroundSuccessors(successor_generator, state_repository, axiom_evaluator)
        .successor_node(start_node, ground_action)
        .pack()
    )
    assert planning.ground.get_node(state_repository, packed_binding_successor) == binding_successor
    assert planning.ground.get_node(state_repository, packed_action_successor) == binding_successor
    assert any(
        labeled_successor.label() == binding and labeled_successor.node() == binding_successor
        for labeled_successor in labeled_successor_nodes
    )


def _check_action_binding_api_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task)
    start_node = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node()

    bindings = LiftedSuccessors.applicable_action_bindings(successor_generator, start_node)
    labeled_successor_nodes = LiftedSuccessors(
        successor_generator, state_repository, axiom_evaluator
    ).labeled_successor_nodes(start_node)

    assert len(bindings) == len(labeled_successor_nodes)
    assert bindings

    binding = bindings[0]
    ground_action = successor_generator.ground_action(binding)
    binding_successor = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).successor_node(
        start_node, binding
    )
    ground_action_successor = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).successor_node(
        start_node, ground_action
    )

    assert type(binding) is formalism.RowActionLifted
    assert successor_generator.ground_action(binding) == ground_action
    assert ground_action.action().arity() == len(list(ground_action.row().resolve_objects()))
    assert binding_successor == ground_action_successor
    packed_binding_successor = (
        LiftedSuccessors(successor_generator, state_repository, axiom_evaluator)
        .successor_node(start_node, binding)
        .pack()
    )
    packed_action_successor = (
        LiftedSuccessors(successor_generator, state_repository, axiom_evaluator)
        .successor_node(start_node, ground_action)
        .pack()
    )
    assert planning.lifted.get_node(state_repository, packed_binding_successor) == binding_successor
    assert planning.lifted.get_node(state_repository, packed_action_successor) == binding_successor
    assert any(
        labeled_successor.label() == binding and labeled_successor.node() == binding_successor
        for labeled_successor in labeled_successor_nodes
    )


def _check_action_schema_successors_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_ground(task)
    actions = list(task.get_task().domain().actions())
    initial_node = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node()
    initial_successors = GroundSuccessors(
        successor_generator, state_repository, axiom_evaluator
    ).labeled_successor_nodes(initial_node)
    picked_node = next(
        successor.node()
        for successor in initial_successors
        if successor.label().relation().original_name() == "pick"
    )

    for node in (initial_node, picked_node):
        all_successors = GroundSuccessors(
            successor_generator, state_repository, axiom_evaluator
        ).labeled_successor_nodes(node)
        all_bindings = GroundSuccessors.applicable_action_bindings(successor_generator, node)
        successors = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).labeled_successor_nodes(node)
        packed = [successor.pack() for successor in successors]
        packed_plan = planning.ground.deterministic.PlanBuilder(start_node=node, labeled_succ_nodes=successors).view(state_repository)
        assert [
            (step.label(), planning.ground.get_node(state_repository, record.node))
            for step, record in zip(packed_plan.labeled_succ_nodes(), packed, strict=True)
        ] == [(successor.label(), successor.node()) for successor in all_successors]
        packed_nodes = [
            successor.pack()
            for successor in GroundSuccessors(successor_generator, state_repository, axiom_evaluator).successor_nodes(
                node
            )
        ]
        assert [planning.ground.get_node(state_repository, successor) for successor in packed_nodes] == [
            successor.node() for successor in all_successors
        ]
        assert Counter(
            GroundSuccessors(successor_generator, state_repository, axiom_evaluator).successor_nodes(node)
        ) == Counter(successor.node() for successor in all_successors)
        for action in actions:
            expected = {
                successor.label(): successor.node()
                for successor in all_successors
                if successor.label().relation() == action
            }
            selected = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).labeled_successor_nodes(
                node, action=action
            )
            assert {successor.label(): successor.node() for successor in selected} == expected
            assert set(GroundSuccessors.applicable_action_bindings(successor_generator, node, action)) == {
                binding for binding in all_bindings if binding.relation() == action
            }
            assert Counter(
                GroundSuccessors(successor_generator, state_repository, axiom_evaluator).successor_nodes(
                    node, action=action
                )
            ) == Counter(expected.values())
            successors = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).labeled_successor_nodes(node, action=action)
            packed = [successor.pack() for successor in successors]
            packed_plan = planning.ground.deterministic.PlanBuilder(start_node=node, labeled_succ_nodes=successors).view(state_repository)
            assert {
                step.label(): planning.ground.get_node(state_repository, record.node)
                for step, record in zip(packed_plan.labeled_succ_nodes(), packed, strict=True)
            } == expected
            packed_nodes = [
                successor.pack()
                for successor in GroundSuccessors(
                    successor_generator, state_repository, axiom_evaluator
                ).successor_nodes(node, action=action)
            ]
            assert Counter(
                planning.ground.get_node(state_repository, successor) for successor in packed_nodes
            ) == Counter(expected.values())
            if action.original_name() == "drop":
                assert bool(expected) == (node == picked_node)


def _check_action_schema_successors_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task)
    actions = list(task.get_task().domain().actions())
    initial_node = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node()
    initial_successors = LiftedSuccessors(
        successor_generator, state_repository, axiom_evaluator
    ).labeled_successor_nodes(initial_node)
    picked_node = next(
        successor.node()
        for successor in initial_successors
        if successor.label().relation().original_name() == "pick"
    )

    for node in (initial_node, picked_node):
        all_successors = LiftedSuccessors(
            successor_generator, state_repository, axiom_evaluator
        ).labeled_successor_nodes(node)
        all_bindings = LiftedSuccessors.applicable_action_bindings(successor_generator, node)
        successors = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).labeled_successor_nodes(node)
        packed = [successor.pack() for successor in successors]
        packed_plan = planning.lifted.deterministic.PlanBuilder(start_node=node, labeled_succ_nodes=successors).view(state_repository)
        assert [
            (step.label(), planning.lifted.get_node(state_repository, record.node))
            for step, record in zip(packed_plan.labeled_succ_nodes(), packed, strict=True)
        ] == [(successor.label(), successor.node()) for successor in all_successors]
        packed_nodes = [
            successor.pack()
            for successor in LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).successor_nodes(
                node
            )
        ]
        assert [planning.lifted.get_node(state_repository, successor) for successor in packed_nodes] == [
            successor.node() for successor in all_successors
        ]
        assert Counter(
            LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).successor_nodes(node)
        ) == Counter(successor.node() for successor in all_successors)
        for action in actions:
            expected = {
                successor.label(): successor.node()
                for successor in all_successors
                if successor.label().relation() == action
            }
            selected = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).labeled_successor_nodes(
                node, action=action
            )
            assert {successor.label(): successor.node() for successor in selected} == expected
            assert set(LiftedSuccessors.applicable_action_bindings(successor_generator, node, action)) == {
                binding for binding in all_bindings if binding.relation() == action
            }
            assert Counter(
                LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).successor_nodes(
                    node, action=action
                )
            ) == Counter(expected.values())
            successors = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).labeled_successor_nodes(node, action=action)
            packed = [successor.pack() for successor in successors]
            packed_plan = planning.lifted.deterministic.PlanBuilder(start_node=node, labeled_succ_nodes=successors).view(state_repository)
            assert {
                step.label(): planning.lifted.get_node(state_repository, record.node)
                for step, record in zip(packed_plan.labeled_succ_nodes(), packed, strict=True)
            } == expected
            packed_nodes = [
                successor.pack()
                for successor in LiftedSuccessors(
                    successor_generator, state_repository, axiom_evaluator
                ).successor_nodes(node, action=action)
            ]
            assert Counter(
                planning.lifted.get_node(state_repository, successor) for successor in packed_nodes
            ) == Counter(expected.values())
            if action.original_name() == "drop":
                assert bool(expected) == (node == picked_node)


def _check_unlabeled_successor_callbacks_ground(
    generator: planning.ground.SuccessorGenerator,
    node: GroundNode,
    action: formalism.ActionLifted | None,
    state_repository: planning.ground.StateRepository,
    axiom_evaluator: planning.ground.AxiomEvaluator,
) -> None:
    if action is None:
        _check_callback_visits(
            GroundSuccessors(generator, state_repository, axiom_evaluator).successor_nodes(node),
            lambda callback: GroundSuccessors(generator, state_repository, axiom_evaluator).for_each_successor_node(
                node, callback
            ),
            lambda successor: successor,
        )
    else:
        _check_callback_visits(
            GroundSuccessors(generator, state_repository, axiom_evaluator).successor_nodes(node, action=action),
            lambda callback: GroundSuccessors(generator, state_repository, axiom_evaluator).for_each_successor_node(
                node, callback, action=action
            ),
            lambda successor: successor,
        )


def _check_unlabeled_successor_callbacks_lifted(
    generator: planning.lifted.SuccessorGenerator,
    node: LiftedNode,
    action: formalism.ActionLifted | None,
    state_repository: planning.lifted.StateRepository,
    axiom_evaluator: planning.lifted.AxiomEvaluator,
) -> None:
    if action is None:
        _check_callback_visits(
            LiftedSuccessors(generator, state_repository, axiom_evaluator).successor_nodes(node),
            lambda callback: LiftedSuccessors(generator, state_repository, axiom_evaluator).for_each_successor_node(
                node, callback
            ),
            lambda successor: successor,
        )
    else:
        _check_callback_visits(
            LiftedSuccessors(generator, state_repository, axiom_evaluator).successor_nodes(node, action=action),
            lambda callback: LiftedSuccessors(generator, state_repository, axiom_evaluator).for_each_successor_node(
                node, callback, action=action
            ),
            lambda successor: successor,
        )


def _check_labeled_successor_callbacks_ground(
    generator: planning.ground.SuccessorGenerator,
    node: GroundNode,
    action: formalism.ActionLifted | None,
    state_repository: planning.ground.StateRepository,
    axiom_evaluator: planning.ground.AxiomEvaluator,
) -> None:
    if action is None:
        _check_callback_visits(
            GroundSuccessors(generator, state_repository, axiom_evaluator).labeled_successor_nodes(node),
            lambda callback: GroundSuccessors(
                generator, state_repository, axiom_evaluator
            ).for_each_labeled_successor_node(node, callback),
            lambda successor: (successor.label(), successor.node()),
        )
    else:
        _check_callback_visits(
            GroundSuccessors(generator, state_repository, axiom_evaluator).labeled_successor_nodes(node, action=action),
            lambda callback: GroundSuccessors(
                generator, state_repository, axiom_evaluator
            ).for_each_labeled_successor_node(node, callback, action=action),
            lambda successor: (successor.label(), successor.node()),
        )


def _check_labeled_successor_callbacks_lifted(
    generator: planning.lifted.SuccessorGenerator,
    node: LiftedNode,
    action: formalism.ActionLifted | None,
    state_repository: planning.lifted.StateRepository,
    axiom_evaluator: planning.lifted.AxiomEvaluator,
) -> None:
    if action is None:
        _check_callback_visits(
            LiftedSuccessors(generator, state_repository, axiom_evaluator).labeled_successor_nodes(node),
            lambda callback: LiftedSuccessors(
                generator, state_repository, axiom_evaluator
            ).for_each_labeled_successor_node(node, callback),
            lambda successor: (successor.label(), successor.node()),
        )
    else:
        _check_callback_visits(
            LiftedSuccessors(generator, state_repository, axiom_evaluator).labeled_successor_nodes(node, action=action),
            lambda callback: LiftedSuccessors(
                generator, state_repository, axiom_evaluator
            ).for_each_labeled_successor_node(node, callback, action=action),
            lambda successor: (successor.label(), successor.node()),
        )


def _check_successor_callbacks_ground(task: planning.ground.Task, *, labeled: bool) -> None:
    state_repository, axiom_evaluator, generator = make_components_ground(task)
    node = GroundSuccessors(generator, state_repository, axiom_evaluator).initial_node()
    check = _check_labeled_successor_callbacks_ground if labeled else _check_unlabeled_successor_callbacks_ground

    for action in [None, *task.get_task().domain().actions()]:
        check(generator, node, action, state_repository, axiom_evaluator)

    successors = GroundSuccessors(generator, state_repository, axiom_evaluator)
    visit = successors.for_each_labeled_successor_node if labeled else successors.for_each_successor_node
    with pytest.raises(RuntimeError, match="callback failure"):
        visit(node, _fail_callback)
    assert visit(node, _continue_callback)


def _check_successor_callbacks_lifted(task: planning.lifted.Task, *, labeled: bool) -> None:
    state_repository, axiom_evaluator, generator = make_components_lifted(task)
    node = LiftedSuccessors(generator, state_repository, axiom_evaluator).initial_node()
    check = _check_labeled_successor_callbacks_lifted if labeled else _check_unlabeled_successor_callbacks_lifted

    for action in [None, *task.get_task().domain().actions()]:
        check(generator, node, action, state_repository, axiom_evaluator)

    successors = LiftedSuccessors(generator, state_repository, axiom_evaluator)
    visit = successors.for_each_labeled_successor_node if labeled else successors.for_each_successor_node
    with pytest.raises(RuntimeError, match="callback failure"):
        visit(node, _fail_callback)
    assert visit(node, _continue_callback)


def _collect_successor_callback_values_ground(
    task: planning.ground.Task, *, labeled: bool
) -> tuple[list[object], list[str]]:
    state_repository, axiom_evaluator, generator = make_components_ground(task)
    node = GroundSuccessors(generator, state_repository, axiom_evaluator).initial_node()
    successors = GroundSuccessors(generator, state_repository, axiom_evaluator)
    visit = successors.for_each_labeled_successor_node if labeled else successors.for_each_successor_node
    seen: list[object] = []

    def record(successor: object) -> bool:
        seen.append(successor)
        return True

    assert visit(node, record)
    return seen, [str(successor) for successor in seen]


def _collect_successor_callback_values_lifted(
    task: planning.lifted.Task, *, labeled: bool
) -> tuple[list[object], list[str]]:
    state_repository, axiom_evaluator, generator = make_components_lifted(task)
    node = LiftedSuccessors(generator, state_repository, axiom_evaluator).initial_node()
    successors = LiftedSuccessors(generator, state_repository, axiom_evaluator)
    visit = successors.for_each_labeled_successor_node if labeled else successors.for_each_successor_node
    seen: list[object] = []

    def record(successor: object) -> bool:
        seen.append(successor)
        return True

    assert visit(node, record)
    return seen, [str(successor) for successor in seen]


def test_successor_generator_accepts_explicit_repository_for_node_lookup() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_node_lookup_ground(ground_task)
    _check_node_lookup_lifted(lifted_task)


def test_successor_generators_expose_uniform_action_binding_api() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_action_binding_api_ground(ground_task)
    _check_action_binding_api_lifted(lifted_task)


def test_successor_generation_for_a_lifted_action_schema() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_action_schema_successors_ground(ground_task)
    _check_action_schema_successors_lifted(lifted_task)


@pytest.mark.parametrize("family", ["ground", "lifted"])
@pytest.mark.parametrize("labeled", [False, True])
def test_successor_callbacks_match_lists_stop_and_propagate_exceptions(family: str, labeled: bool) -> None:
    ground_task, lifted_task = make_gripper_tasks()
    if family == "ground":
        _check_successor_callbacks_ground(ground_task, labeled=labeled)
    else:
        _check_successor_callbacks_lifted(lifted_task, labeled=labeled)


@pytest.mark.parametrize("family", ["ground", "lifted"])
@pytest.mark.parametrize("labeled", [False, True])
def test_successor_callback_values_retain_their_repository(family: str, labeled: bool) -> None:
    def collect() -> tuple[list[object], list[str]]:
        ground_task, lifted_task = make_gripper_tasks()
        if family == "ground":
            return _collect_successor_callback_values_ground(ground_task, labeled=labeled)
        return _collect_successor_callback_values_lifted(lifted_task, labeled=labeled)

    seen, expected = collect()
    gc.collect()
    assert seen
    assert [str(successor) for successor in seen] == expected
