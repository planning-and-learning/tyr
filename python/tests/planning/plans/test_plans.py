# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from pytyr import planning
from python.tests.planning.support.components import GroundLabeledNode, GroundSuccessors, LiftedLabeledNode, LiftedSuccessors, make_components_ground, make_components_lifted, make_gripper_tasks


def _check_packed_plans_ground(task: planning.ground.Task) -> None:
    state_repository, axiom_evaluator, generator = make_components_ground(task)
    node = GroundSuccessors(generator, state_repository, axiom_evaluator).initial_node()
    first = GroundSuccessors(generator, state_repository, axiom_evaluator).labeled_successor_nodes(node)[0]
    second = GroundSuccessors(generator, state_repository, axiom_evaluator).labeled_successor_nodes(first.node())[0]
    step_lists: tuple[list[GroundLabeledNode], ...] = ([], [first, second])
    for steps in step_lists:
        expected = [
            (step.label(), step.node().state().pack(), step.node().metric()) for step in steps
        ]
        plan = planning.ground.deterministic.PlanBuilder(start_node=node, labeled_succ_nodes=list(steps)).view(state_repository)
        assert plan.start_node().state().pack() == node.state().pack()
        assert plan.start_node().metric() == node.metric()
        assert plan.length() == len(steps)
        assert plan.empty() == (not steps)
        assert plan.cost() == (steps[-1].node().metric() if steps else 0)
        assert [
            (step.label(), step.node().state().pack(), step.node().metric())
            for step in plan.labeled_succ_nodes()
        ] == expected
        assert plan.goal_node().state().pack() == (steps[-1].node() if steps else node).state().pack()
        assert repr(plan) == str(plan)


def _check_packed_plans_lifted(task: planning.lifted.Task) -> None:
    state_repository, axiom_evaluator, generator = make_components_lifted(task)
    node = LiftedSuccessors(generator, state_repository, axiom_evaluator).initial_node()
    first = LiftedSuccessors(generator, state_repository, axiom_evaluator).labeled_successor_nodes(node)[0]
    second = LiftedSuccessors(generator, state_repository, axiom_evaluator).labeled_successor_nodes(first.node())[0]
    step_lists: tuple[list[LiftedLabeledNode], ...] = ([], [first, second])
    for steps in step_lists:
        expected = [
            (step.label(), step.node().state().pack(), step.node().metric()) for step in steps
        ]
        plan = planning.lifted.deterministic.PlanBuilder(start_node=node, labeled_succ_nodes=list(steps)).view(state_repository)
        assert plan.start_node().state().pack() == node.state().pack()
        assert plan.start_node().metric() == node.metric()
        assert plan.length() == len(steps)
        assert plan.empty() == (not steps)
        assert plan.cost() == (steps[-1].node().metric() if steps else 0)
        assert [
            (step.label(), step.node().state().pack(), step.node().metric())
            for step in plan.labeled_succ_nodes()
        ] == expected
        assert plan.goal_node().state().pack() == (steps[-1].node() if steps else node).state().pack()
        assert repr(plan) == str(plan)


def test_packed_plans_preserve_empty_and_multistep_plans() -> None:
    ground_task, lifted_task = make_gripper_tasks()
    _check_packed_plans_ground(ground_task)
    _check_packed_plans_lifted(lifted_task)
