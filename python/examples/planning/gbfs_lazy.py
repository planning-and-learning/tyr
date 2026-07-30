"""
Custom lazy Greedy Best-First Search (GBFS) with customization points explained.

Tyr also provides an off-the-shelf implementation, which is almost identical
to the astar_eager.py example. Please familiarize yourself with astar_eager.py
before exploring this example.

Example usage (benchmark files are provided by the pypddl-datasets package):

    GRIPPER=$(python3 -c "import pypddl_datasets; print(pypddl_datasets.fetch_domain('classical/tests/gripper').path)")
    python3 python/examples/planning/gbfs_lazy.py -d "$GRIPPER/domain.pddl" -p "$GRIPPER/test-1.pddl"

Author: Dominik Drexler (dominik.drexler@liu.se)
"""

import argparse

from pathlib import Path
from queue import PriorityQueue
from dataclasses import dataclass
from enum import Enum

from pyyggdrasil.execution import ExecutionContext

from pypddl.formalism import ParserOptions
from pytyr.formalism.planning import Parser

from pytyr.planning import SearchStatus

from pytyr.planning.lifted import (
    StateIndex,
    SearchResult,
    AxiomEvaluatorFactory,
    StateRepositoryFactory,
    SuccessorGenerator,
    SuccessorGeneratorFactory,
    Heuristic,
    FFRPGHeuristic,
    ConjunctiveGoalStrategy,
    Node,
    LabeledNode,
    Plan,
    Task,
)


class SearchNodeStatus(Enum):
    NEW = 0
    OPEN = 1
    CLOSED = 2
    DEADEND = 3
    GOAL = 4


@dataclass
class SearchNode:
    g_value: float
    parent: StateIndex | None
    status: SearchNodeStatus


def backtrack_plan(
    goal_node: Node,
    search_nodes: dict[StateIndex, SearchNode],
    successor_generator: SuccessorGenerator,
) -> Plan:
    """
    Backtracks from the goal search node to compute the labeled state trajectory.
    """
    cur_state_index = goal_node.get_state().get_index()
    backward_state_trajectory: list[StateIndex] = []

    while cur_state_index is not None:
        backward_state_trajectory.append(cur_state_index)
        search_node = search_nodes[cur_state_index]
        cur_state_index = search_node.parent

    forward_state_trajectory = list(reversed(backward_state_trajectory))

    assert forward_state_trajectory

    start_node = successor_generator.get_node(forward_state_trajectory[0])
    node = start_node
    labeled_succ_nodes: list[LabeledNode] = []

    for i in range(len(forward_state_trajectory) - 1):
        for labeled_succ_node in successor_generator.get_labeled_successor_nodes(node):
            succ_node = labeled_succ_node.node
            succ_state = succ_node.get_state()
            succ_state_index = succ_state.get_index()

            if succ_state_index == forward_state_trajectory[i + 1]:
                labeled_succ_nodes.append(labeled_succ_node)
                node = labeled_succ_node.node

    return Plan(start_node, labeled_succ_nodes)


def find_solution(
    task: Task, successor_generator: SuccessorGenerator, heuristic: Heuristic
) -> SearchResult:

    state_repository = successor_generator.get_state_repository()

    goal_strategy = ConjunctiveGoalStrategy(task)

    search_result = SearchResult()

    if not goal_strategy.is_static_goal_satisfied(task):
        search_result.goal_node = None
        search_result.plan = None
        search_result.status = SearchStatus.UNSOLVABLE
        return search_result

    initial_node = successor_generator.get_initial_node()
    initial_state = initial_node.get_state()

    if goal_strategy.is_dynamic_goal_satisfied(initial_state, initial_state):
        search_result.goal_node = initial_node
        search_result.plan = Plan(initial_node)
        search_result.status = SearchStatus.SOLVED
        return search_result

    initial_h_value = heuristic.evaluate(initial_state)
    initial_g_value = initial_node.get_metric()
    initial_state_index = initial_state.get_index()

    search_nodes: dict[StateIndex, SearchNode] = dict()
    search_nodes[initial_state_index] = SearchNode(
        initial_g_value, None, SearchNodeStatus.NEW
    )

    queue: PriorityQueue[tuple[float, float, StateIndex]] = PriorityQueue()
    # Note: we insert cheap state indices to allow memory to be returned to the pool.
    # We can always retrieve a state by calling state_repository.get_registered_state(i) for a StateIndex i.
    queue.put((initial_h_value, initial_g_value, initial_state_index))

    while not queue.empty():
        _, g_value, state_index = queue.get()
        state = state_repository.get_registered_state(state_index)
        node = Node(state, g_value)
        search_node = search_nodes[state_index]

        state_h_value = heuristic.evaluate(state)
        preferred_actions = heuristic.get_preferred_actions()

        search_node.status = SearchNodeStatus.CLOSED

        if state_h_value == float("inf"):
            search_node.status = SearchNodeStatus.DEADEND
            continue

        for labeled_succ_node in successor_generator.get_labeled_successor_nodes(node):
            succ_node = labeled_succ_node.node
            succ_state = succ_node.get_state()
            succ_g_value = succ_node.get_metric()
            succ_state_index = succ_state.get_index()

            if succ_state_index not in search_nodes:
                search_nodes[succ_state_index] = SearchNode(
                    succ_g_value, state_index, SearchNodeStatus.NEW
                )

            succ_search_node = search_nodes[succ_state_index]

            if not succ_search_node.status == SearchNodeStatus.NEW:
                continue

            succ_search_node.parent = state_index

            action = labeled_succ_node.label
            if action in preferred_actions:
                pass  # the heuristic marked this labeled successor node as preferred

            if goal_strategy.is_dynamic_goal_satisfied(initial_state, succ_state):
                search_result.goal_node = succ_node
                search_result.plan = backtrack_plan(
                    succ_node, search_nodes, successor_generator
                )
                search_result.status = SearchStatus.SOLVED
                return search_result

            succ_search_node.status = SearchNodeStatus.OPEN
            queue.put((state_h_value, succ_g_value, succ_state_index))

    search_result.goal_node = None
    search_result.plan = None
    search_result.status = SearchStatus.EXHAUSTED
    return search_result


def main() -> None:
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
    axiom_evaluator_factory = AxiomEvaluatorFactory()
    state_repository_factory = StateRepositoryFactory()
    successor_generator_factory = SuccessorGeneratorFactory()
    axiom_evaluator = axiom_evaluator_factory.create(lifted_task, execution_context)
    state_repository = state_repository_factory.create(lifted_task, axiom_evaluator)
    successor_generator = successor_generator_factory.create(
        lifted_task, execution_context, state_repository
    )

    search_result = find_solution(lifted_task, successor_generator, heuristic)

    print("Search status:", search_result.status)

    plan = search_result.plan

    if plan is not None:
        print(f"Found plan with length {plan.get_length()} and cost {plan.get_cost()}")
        print(plan)
    else:
        print("No solution was found.")


if __name__ == "__main__":
    main()
