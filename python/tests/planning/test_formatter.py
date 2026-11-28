# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from python.tests.planning.support.components import (
    GroundSuccessors,
    LiftedSuccessors,
    make_components_ground,
    make_components_lifted,
    make_gripper_tasks,
)

from pytyr import planning


def _check_repr_matches_str_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_ground(task)
    state = state_repository.get_initial_state(axiom_evaluator)
    node = planning.ground.get_node(state_repository, planning.ground.NodeData(state.pack().index(), 0.0))
    labeled_node = GroundSuccessors(successor_generator, state_repository, axiom_evaluator).labeled_successor_nodes(
        node
    )[0]
    plan = planning.ground.deterministic.PlanBuilder(start_node=node, labeled_succ_nodes=[]).view(state_repository)

    assert repr(state) == str(state)
    assert repr(node) == str(node)
    assert repr(labeled_node) == str(labeled_node)
    assert repr(plan) == str(plan)
    assert plan.length() == 0
    assert plan.empty()


def _check_repr_matches_str_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, successor_generator = make_components_lifted(task)
    state = state_repository.get_initial_state(axiom_evaluator)
    node = planning.lifted.get_node(state_repository, planning.lifted.NodeData(state.pack().index(), 0.0))
    labeled_node = LiftedSuccessors(successor_generator, state_repository, axiom_evaluator).labeled_successor_nodes(
        node
    )[0]
    plan = planning.lifted.deterministic.PlanBuilder(start_node=node, labeled_succ_nodes=[]).view(state_repository)

    assert repr(state) == str(state)
    assert repr(node) == str(node)
    assert repr(labeled_node) == str(labeled_node)
    assert repr(plan) == str(plan)
    assert plan.length() == 0
    assert plan.empty()


def test_planning_state_node_and_plan_repr_matches_str() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_repr_matches_str_ground(ground_task)
    _check_repr_matches_str_lifted(lifted_task)
