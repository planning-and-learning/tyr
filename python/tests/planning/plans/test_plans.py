# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from pytyr import planning
from python.tests.planning.support.components import GroundLabeledNode, GroundSuccessors, LiftedLabeledNode, LiftedSuccessors, make_components_ground, make_components_lifted, make_gripper_tasks


def _check_packed_plans_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, generator = make_components_ground(task)
    node = GroundSuccessors(generator, state_repository, axiom_evaluator).initial_node()
    first = GroundSuccessors(generator, state_repository, axiom_evaluator).labeled_successor_nodes(node)[0]
    second = GroundSuccessors(generator, state_repository, axiom_evaluator).labeled_successor_nodes(first.get_node())[0]
    step_lists: tuple[list[GroundLabeledNode], ...] = ([], [first, second])
    for steps in step_lists:
        expected = [
            (step.get_label(), step.get_node().get_state().pack(), step.get_node().get_metric()) for step in steps
        ]
        plan = planning.ground.PlanBuilder(start_node=node, labeled_succ_nodes=list(steps)).view(state_repository)
        assert plan.get_start_node().get_state().pack() == node.get_state().pack()
        assert plan.get_start_node().get_metric() == node.get_metric()
        assert plan.get_length() == len(steps)
        assert plan.empty() == (not steps)
        assert plan.get_cost() == (steps[-1].get_node().get_metric() if steps else 0)
        assert [
            (step.get_label(), step.get_node().get_state().pack(), step.get_node().get_metric())
            for step in plan.get_labeled_succ_nodes()
        ] == expected
        assert plan.get_goal_node().get_state().pack() == (steps[-1].get_node() if steps else node).get_state().pack()
        assert repr(plan) == str(plan)


def _check_packed_plans_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, generator = make_components_lifted(task)
    node = LiftedSuccessors(generator, state_repository, axiom_evaluator).initial_node()
    first = LiftedSuccessors(generator, state_repository, axiom_evaluator).labeled_successor_nodes(node)[0]
    second = LiftedSuccessors(generator, state_repository, axiom_evaluator).labeled_successor_nodes(first.get_node())[0]
    step_lists: tuple[list[LiftedLabeledNode], ...] = ([], [first, second])
    for steps in step_lists:
        expected = [
            (step.get_label(), step.get_node().get_state().pack(), step.get_node().get_metric()) for step in steps
        ]
        plan = planning.lifted.PlanBuilder(start_node=node, labeled_succ_nodes=list(steps)).view(state_repository)
        assert plan.get_start_node().get_state().pack() == node.get_state().pack()
        assert plan.get_start_node().get_metric() == node.get_metric()
        assert plan.get_length() == len(steps)
        assert plan.empty() == (not steps)
        assert plan.get_cost() == (steps[-1].get_node().get_metric() if steps else 0)
        assert [
            (step.get_label(), step.get_node().get_state().pack(), step.get_node().get_metric())
            for step in plan.get_labeled_succ_nodes()
        ] == expected
        assert plan.get_goal_node().get_state().pack() == (steps[-1].get_node() if steps else node).get_state().pack()
        assert repr(plan) == str(plan)


def test_packed_plans_preserve_empty_and_multistep_plans() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_packed_plans_ground(ground_task)
    _check_packed_plans_lifted(lifted_task)
