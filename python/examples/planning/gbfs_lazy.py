# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Custom lazy Greedy Best-First Search (GBFS) with customization points explained.

Tyr also provides an off-the-shelf implementation, which is almost identical
to the astar_eager.py example. Please familiarize yourself with astar_eager.py
before exploring this example.

Example usage (benchmark files are provided by the pypddl-datasets package):

    GRIPPER=$(python3 -c "import pypddl_datasets; print(pypddl_datasets.fetch_domain('classical/tests/gripper').path)")
    python3 python/examples/planning/gbfs_lazy.py -d "$GRIPPER/domain.pddl" -p "$GRIPPER/test-1.pddl"

Author: Dominik Drexler (dominik.drexler@liu.se)
"""

import argparse
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from queue import PriorityQueue

from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext

from pytyr.formalism import Parser
from pytyr.planning import SearchStatus
from pytyr.planning.lifted import (
    AxiomEvaluator,
    ConjunctiveGoalStrategy,
    FFRPGHeuristic,
    Heuristic,
    LabeledNodeBuilder,
    LabeledNodeBuilderView,
    NodeBuilderView,
    OwnedNodeBuilderView,
    NodeData,
    StateIndex,
    StateRepository,
    SuccessorGenerator,
    SuccessorState,
    Task,
    TransitionLabel,
    deterministic,
    extend_successor,
    find_successor,
    get_initial_node,
    get_node,
    intern_action_binding,
    register_successor,
)
from pytyr.planning.lifted.deterministic import PlanBuilder, PlanBuilderView

# A node a caller holds (owned: returned on its own) or reads in place (borrowed: a labeled node's).
Node = OwnedNodeBuilderView | NodeBuilderView
LabeledNode = LabeledNodeBuilderView


class SearchNodeStatus(Enum):
    """The lifecycle status of a search node."""

    NEW = 0
    OPEN = 1
    CLOSED = 2
    DEADEND = 3
    GOAL = 4


def labeled_successor_nodes(
    successor_generator: SuccessorGenerator,
    node: Node,
    state_repository: StateRepository,
    axiom_evaluator: AxiomEvaluator,
) -> list[LabeledNode]:
    """The successors of `node`, each registered and labeled by its action binding.

    The search is deterministic: one successor state per transition (the `deterministic` environment). The lookup gives a
    state that is registered already its index and derived atoms, the axiom step closes any other under the axioms, and
    register_successor registers it. The binding is interned: a transition's binding is borrowed, valid only during its
    callback.
    """
    successors: list[LabeledNode] = []

    def visit(transition: TransitionLabel, successor_state: SuccessorState) -> bool:
        binding = intern_action_binding(successor_generator, transition.binding)
        find_successor(state_repository, node, successor_state)
        extend_successor(axiom_evaluator, node, successor_state)
        successor = register_successor(state_repository, node, successor_state)
        successors.append(LabeledNodeBuilder(label=binding, node=successor).view(state_repository))
        return True

    deterministic.for_each_successor(successor_generator, node, visit)
    return successors


@dataclass
class SearchNode:
    """The search information of a state: its g-value, the parent state on the best known path, and its status."""

    g_value: float
    parent: StateIndex | None
    status: SearchNodeStatus


def backtrack_plan(
    goal_node: Node,
    search_nodes: dict[StateIndex, SearchNode],
    state_repository: StateRepository,
    axiom_evaluator: AxiomEvaluator,
    successor_generator: SuccessorGenerator,
) -> PlanBuilderView:
    """Backtracks from the goal search node to compute the labeled state trajectory."""
    cur_state_index = goal_node.state().pack().index()
    backward_state_trajectory: list[StateIndex] = []

    while cur_state_index is not None:
        backward_state_trajectory.append(cur_state_index)
        search_node = search_nodes[cur_state_index]
        cur_state_index = search_node.parent

    # The trajectory contains at least the goal state.
    forward_state_trajectory = list(reversed(backward_state_trajectory))

    start_node = get_node(state_repository, forward_state_trajectory[0])
    node = start_node
    labeled_succ_nodes: list[LabeledNode] = []

    for i in range(len(forward_state_trajectory) - 1):
        for labeled_succ_node in labeled_successor_nodes(successor_generator, node, state_repository, axiom_evaluator):
            succ_node = labeled_succ_node.node()
            succ_state = succ_node.state()
            succ_state_index = succ_state.pack().index()

            if succ_state_index == forward_state_trajectory[i + 1]:
                labeled_succ_nodes.append(labeled_succ_node)
                node = labeled_succ_node.node()

    return PlanBuilder(start_node=start_node, labeled_succ_nodes=labeled_succ_nodes).view(state_repository)


@dataclass
class SearchResult:
    """The outcome of the search: its status and, if solved, the plan."""

    status: SearchStatus
    plan: PlanBuilderView | None = None


def find_solution(
    task: Task,
    state_repository: StateRepository,
    axiom_evaluator: AxiomEvaluator,
    successor_generator: SuccessorGenerator,
    heuristic: Heuristic,
) -> SearchResult:
    """Run lazy GBFS from the initial state until a state satisfies the task's goal."""
    goal_strategy = ConjunctiveGoalStrategy(task)

    if not goal_strategy.is_static_goal_satisfied(task):
        return SearchResult(SearchStatus.UNSOLVABLE)


    initial_node = get_initial_node(state_repository, axiom_evaluator)
    initial_state = initial_node.state()

    if goal_strategy.is_dynamic_goal_satisfied(initial_state, initial_state):
        return SearchResult(
            SearchStatus.SOLVED, PlanBuilder(start_node=initial_node, labeled_succ_nodes=[]).view(state_repository)
        )

    initial_h_value = heuristic.evaluate(initial_state)
    initial_g_value = initial_node.metric()
    initial_state_index = initial_state.pack().index()

    search_nodes: dict[StateIndex, SearchNode] = {}
    search_nodes[initial_state_index] = SearchNode(initial_g_value, None, SearchNodeStatus.NEW)

    queue: PriorityQueue[tuple[float, float, StateIndex]] = PriorityQueue()
    # Note: we insert cheap state indices to allow memory to be returned to the pool.
    # We can always retrieve a state by calling state_repository.get_registered_state(i) for a StateIndex i.
    queue.put((initial_h_value, initial_g_value, initial_state_index))

    while not queue.empty():
        _, g_value, state_index = queue.get()
        node = get_node(state_repository, NodeData(state_index, g_value))
        state = node.state()
        search_node = search_nodes[state_index]

        state_h_value = heuristic.evaluate(state)
        relaxed_plan = heuristic.get_relaxed_plan()

        search_node.status = SearchNodeStatus.CLOSED

        if state_h_value == float("inf"):
            search_node.status = SearchNodeStatus.DEADEND
            continue

        for labeled_succ_node in labeled_successor_nodes(successor_generator, node, state_repository, axiom_evaluator):
            succ_node = labeled_succ_node.node()
            succ_state = succ_node.state()
            succ_g_value = succ_node.metric()
            succ_state_index = succ_state.pack().index()

            if succ_state_index not in search_nodes:
                search_nodes[succ_state_index] = SearchNode(succ_g_value, state_index, SearchNodeStatus.NEW)

            succ_search_node = search_nodes[succ_state_index]

            if succ_search_node.status != SearchNodeStatus.NEW:
                continue

            succ_search_node.parent = state_index

            action = labeled_succ_node.label()
            if relaxed_plan is not None and action in relaxed_plan:
                pass  # the action is applicable and in the relaxed plan: a preferred successor

            if goal_strategy.is_dynamic_goal_satisfied(initial_state, succ_state):
                return SearchResult(
                    SearchStatus.SOLVED,
                    backtrack_plan(succ_node, search_nodes, state_repository, axiom_evaluator, successor_generator),
                )

            succ_search_node.status = SearchNodeStatus.OPEN
            queue.put((state_h_value, succ_g_value, succ_state_index))

    return SearchResult(SearchStatus.EXHAUSTED)


def main() -> None:
    """Run the custom lazy GBFS with the FF heuristic on the given task and print the plan."""
    arg_parser = argparse.ArgumentParser(description="GBFS Lazy Search.")
    arg_parser.add_argument(
        "-d",
        "--domain-filepath",
        type=Path,
        required=True,
        help="Path to a PDDL domain file.",
    )
    arg_parser.add_argument(
        "-p",
        "--task-filepath",
        type=Path,
        required=True,
        help="Path to PDDL task file.",
    )
    args = arg_parser.parse_args()

    domain_filepath: Path = args.domain_filepath
    task_filepath: Path = args.task_filepath

    execution_context = ExecutionContext(2)
    parser_options = ParserOptions()
    parser = Parser(str(domain_filepath), parser_options)
    lifted_task = Task(parser.parse_task(str(task_filepath), parser_options))
    heuristic = FFRPGHeuristic(lifted_task, execution_context)
    axiom_evaluator = AxiomEvaluator(lifted_task, execution_context)
    state_repository = StateRepository(lifted_task)
    successor_generator = SuccessorGenerator(lifted_task, execution_context)

    search_result = find_solution(lifted_task, state_repository, axiom_evaluator, successor_generator, heuristic)

    print("Search status:", search_result.status)

    plan = search_result.plan

    if plan is not None:
        print(f"Found plan with length {plan.length()} and cost {plan.cost()}")
        print(plan)
    else:
        print("No solution was found.")


if __name__ == "__main__":
    main()
