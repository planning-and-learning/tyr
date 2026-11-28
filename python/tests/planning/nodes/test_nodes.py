# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc

import pytest
from python.tests.planning.support.components import (
    GroundSuccessors,
    LiftedSuccessors,
    interned_label,
    make_components_ground,
    make_components_lifted,
    make_gripper_tasks,
)

from pytyr import formalism, planning


def _check_packed_nodes_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_ground(task)
    successors = GroundSuccessors(successor_generator, state_repository, axiom_evaluator)
    initial_node = successors.initial_node()
    successor = successors.labeled_successor_nodes(initial_node)[0]
    node = planning.ground.get_node(
        state_repository, planning.ground.NodeData(successor.node().state().pack().index(), 17.5)
    )
    labeled = planning.ground.LabeledNodeBuilder(label=successor.label(), node=node).view(state_repository)
    state_text = str(node.state())
    label_text = str(labeled.label())
    packed = node.pack()
    packed_labeled = labeled.pack()

    assert type(packed) is planning.ground.NodeData
    assert packed.metric == 17.5
    assert packed == planning.ground.NodeData(node.state().pack().index(), 17.5)
    assert planning.ground.get_node(state_repository, packed) == node
    assert type(packed_labeled) is planning.ground.LabeledNodeData
    assert packed_labeled.node == packed
    assert repr(packed) == str(packed)
    assert repr(packed_labeled) == str(packed_labeled)
    plan = planning.ground.deterministic.PlanBuilder(start_node=initial_node, labeled_succ_nodes=[labeled]).view(
        state_repository
    )

    del initial_node, successor, successors, node, labeled, state_repository, axiom_evaluator, successor_generator
    gc.collect()

    step = plan.labeled_succ_nodes()[0]
    assert type(step) is planning.ground.LabeledNodeBuilderView
    assert step.node().metric() == 17.5
    assert str(step.node().state()) == state_text
    assert str(step.label()) == label_text


def _check_packed_nodes_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task)
    successors = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator)
    initial_node = successors.initial_node()
    successor = successors.labeled_successor_nodes(initial_node)[0]
    node = planning.lifted.get_node(
        state_repository, planning.lifted.NodeData(successor.node().state().pack().index(), 17.5)
    )
    labeled = planning.lifted.LabeledNodeBuilder(label=successor.label(), node=node).view(state_repository)
    state_text = str(node.state())
    label_text = str(labeled.label())
    packed = node.pack()
    packed_labeled = labeled.pack()

    assert type(packed) is planning.lifted.NodeData
    assert packed.metric == 17.5
    assert packed == planning.lifted.NodeData(node.state().pack().index(), 17.5)
    assert planning.lifted.get_node(state_repository, packed) == node
    assert type(packed_labeled) is planning.lifted.LabeledNodeData
    assert packed_labeled.node == packed
    assert repr(packed) == str(packed)
    assert repr(packed_labeled) == str(packed_labeled)
    plan = planning.lifted.deterministic.PlanBuilder(start_node=initial_node, labeled_succ_nodes=[labeled]).view(
        state_repository
    )

    del initial_node, successor, successors, node, labeled, state_repository, axiom_evaluator, successor_generator
    gc.collect()

    step = plan.labeled_succ_nodes()[0]
    assert type(step) is planning.lifted.LabeledNodeBuilderView
    assert step.node().metric() == 17.5
    assert str(step.node().state()) == state_text
    assert str(step.label()) == label_text


def _check_packed_node_keeps_its_state_repository_alive_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_ground(task)
    initial_node = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node()
    plan = planning.ground.deterministic.PlanBuilder(start_node=initial_node, labeled_succ_nodes=[]).view(
        state_repository
    )
    state_text = str(initial_node.state())
    del initial_node, state_repository, axiom_evaluator, successor_generator
    gc.collect()

    assert str(plan.start_node().state()) == state_text


def _check_packed_node_keeps_its_state_repository_alive_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task)
    initial_node = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node()
    plan = planning.lifted.deterministic.PlanBuilder(start_node=initial_node, labeled_succ_nodes=[]).view(
        state_repository
    )
    state_text = str(initial_node.state())
    del initial_node, state_repository, axiom_evaluator, successor_generator
    gc.collect()

    assert str(plan.start_node().state()) == state_text


def _first_successor_label_ground(task: planning.ground.Task, *, packed: bool) -> tuple[formalism.RowActionLifted, str]:
    state_repository, axiom_evaluator, successor_generator = make_components_ground(task)
    initial_node = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node()
    labeled = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).labeled_successor_nodes(
        initial_node
    )[0]
    plan = planning.ground.deterministic.PlanBuilder(start_node=initial_node, labeled_succ_nodes=[labeled]).view(
        state_repository
    )
    label = interned_label(plan.labeled_succ_nodes()[0] if packed else labeled)
    return label, str(label)


def _first_successor_label_lifted(task: planning.lifted.Task, *, packed: bool) -> tuple[formalism.RowActionLifted, str]:
    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task)
    initial_node = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node()
    labeled = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).labeled_successor_nodes(
        initial_node
    )[0]
    plan = planning.lifted.deterministic.PlanBuilder(start_node=initial_node, labeled_succ_nodes=[labeled]).view(
        state_repository
    )
    label = interned_label(plan.labeled_succ_nodes()[0] if packed else labeled)
    return label, str(label)


def _check_node_equality_and_hash_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, _ = make_components_ground(task)
    state = state_repository.get_initial_state(axiom_evaluator)

    first_node = planning.ground.get_node(state_repository, planning.ground.NodeData(state.pack().index(), 1.0))
    second_node = planning.ground.get_node(state_repository, planning.ground.NodeData(state.pack().index(), 1.0))
    different_metric_node = planning.ground.get_node(
        state_repository, planning.ground.NodeData(state.pack().index(), 2.0)
    )

    assert first_node == second_node
    assert hash(first_node) == hash(second_node)
    assert first_node != different_metric_node


def _check_node_equality_and_hash_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, _ = make_components_lifted(task)
    state = state_repository.get_initial_state(axiom_evaluator)

    first_node = planning.lifted.get_node(state_repository, planning.lifted.NodeData(state.pack().index(), 1.0))
    second_node = planning.lifted.get_node(state_repository, planning.lifted.NodeData(state.pack().index(), 1.0))
    different_metric_node = planning.lifted.get_node(
        state_repository, planning.lifted.NodeData(state.pack().index(), 2.0)
    )

    assert first_node == second_node
    assert hash(first_node) == hash(second_node)
    assert first_node != different_metric_node


def _check_labeled_node_plan_construction_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_ground(task)
    start_node = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node()
    labeled_successor_nodes = GroundSuccessors(
        successor_generator, state_repository, axiom_evaluator
    ).labeled_successor_nodes(start_node)

    assert labeled_successor_nodes
    first_labeled_successor = labeled_successor_nodes[0]
    assert type(first_labeled_successor.label()) is formalism.RowActionLifted
    label = interned_label(first_labeled_successor)
    assert (
        GroundSuccessors(successor_generator, state_repository, axiom_evaluator).successor_node(start_node, label)
        == first_labeled_successor.node()
    )
    assert (
        GroundSuccessors(successor_generator, state_repository, axiom_evaluator).successor_node(
            start_node, successor_generator.ground_action(label)
        )
        == first_labeled_successor.node()
    )

    labeled_node = planning.ground.LabeledNodeBuilder(
        label=label,
        node=first_labeled_successor.node(),
    ).view(state_repository)
    plan = planning.ground.deterministic.PlanBuilder(start_node=start_node, labeled_succ_nodes=[labeled_node]).view(
        state_repository
    )

    assert plan.length() == 1
    assert not plan.empty()
    assert plan.cost() == labeled_node.node().metric()
    assert plan.start_node().state().pack() == start_node.state().pack()
    assert interned_label(plan.labeled_succ_nodes()[0]) == interned_label(labeled_node)
    assert plan.labeled_succ_nodes()[0].node().state().pack() == labeled_node.node().state().pack()


def _check_labeled_node_plan_construction_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task)
    start_node = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).initial_node()
    labeled_successor_nodes = LiftedSuccessors(
        successor_generator, state_repository, axiom_evaluator
    ).labeled_successor_nodes(start_node)

    assert labeled_successor_nodes
    first_labeled_successor = labeled_successor_nodes[0]
    assert type(first_labeled_successor.label()) is formalism.RowActionLifted
    label = interned_label(first_labeled_successor)
    assert (
        LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).successor_node(start_node, label)
        == first_labeled_successor.node()
    )
    assert (
        LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).successor_node(
            start_node, successor_generator.ground_action(label)
        )
        == first_labeled_successor.node()
    )

    labeled_node = planning.lifted.LabeledNodeBuilder(
        label=label,
        node=first_labeled_successor.node(),
    ).view(state_repository)
    plan = planning.lifted.deterministic.PlanBuilder(start_node=start_node, labeled_succ_nodes=[labeled_node]).view(
        state_repository
    )

    assert plan.length() == 1
    assert not plan.empty()
    assert plan.cost() == labeled_node.node().metric()
    assert plan.start_node().state().pack() == start_node.state().pack()
    assert interned_label(plan.labeled_succ_nodes()[0]) == interned_label(labeled_node)
    assert plan.labeled_succ_nodes()[0].node().state().pack() == labeled_node.node().state().pack()


def test_packed_nodes_preserve_state_metric_and_label() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_packed_nodes_ground(ground_task)
    _check_packed_nodes_lifted(lifted_task)


def test_packed_node_keeps_its_state_repository_alive() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_packed_node_keeps_its_state_repository_alive_ground(ground_task)
    _check_packed_node_keeps_its_state_repository_alive_lifted(lifted_task)


@pytest.mark.parametrize("family", ["ground", "lifted"])
@pytest.mark.parametrize("packed", [False, True])
def test_labeled_node_label_keeps_owning_node_alive(family: str, packed: bool) -> None:
    def make_label() -> tuple[formalism.RowActionLifted, str]:
        ground_task, lifted_task = make_gripper_tasks()
        if family == "ground":
            return _first_successor_label_ground(ground_task, packed=packed)
        return _first_successor_label_lifted(lifted_task, packed=packed)

    label, expected = make_label()
    gc.collect()
    assert str(label) == expected


def test_node_python_equality_and_hash_follow_state_and_metric() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_node_equality_and_hash_ground(ground_task)
    _check_node_equality_and_hash_lifted(lifted_task)


def test_labeled_node_is_constructible_for_plan_construction() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_labeled_node_plan_construction_ground(ground_task)
    _check_labeled_node_plan_construction_lifted(lifted_task)
