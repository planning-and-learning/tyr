# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Off-the-shelf A* eager with customization points explained.

Example usage (benchmark files are provided by the pypddl-datasets package):

    GRIPPER=$(python3 -c "import pypddl_datasets; print(pypddl_datasets.fetch_domain('classical/tests/gripper').path)")
    python3 python/examples/planning/astar_eager.py -d "$GRIPPER/domain.pddl" -p "$GRIPPER/test-1.pddl"

Author: Dominik Drexler (dominik.drexler@liu.se)
"""

import argparse
from pathlib import Path

from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext
from typing_extensions import override

from pytyr.formalism.planning import GroundConjunctiveCondition, Parser
from pytyr.planning import SearchStatus, Statistics

# Note: we can easily switch between lifted and ground planning by swapping the submodule,
# e.g., from pytyr.planning.ground import ..., and from pytyr.planning.ground.astar_eager import ...
# However, some heuristics, like MaxRPGHeuristic might not be available yet.
from pytyr.planning.lifted import (
    AxiomEvaluatorFactory,
    ConjunctiveGoalStrategy,
    GoalCountHeuristic,
    GoalStrategy,
    Heuristic,
    Node,
    Plan,
    PruningStrategy,
    State,
    StateRepositoryFactory,
    SuccessorGeneratorFactory,
    Task,
)
from pytyr.planning.lifted.astar_eager import (
    DefaultEventHandler,
    EventHandler,
    Options,
    find_solution,
)

# Lazy Greedy Best-First Search (GBFS) exposes an almost identical interface,
# although some options and EventHandler events differ slightly.
# To switch the search algorithm, typically only the import above needs to be changed:
# import Options, EventHandler, DefaultEventHandler, and find_solution from pytyr.planning.lifted.gbfs_lazy instead.


class CustomHeuristic(Heuristic):
    """CustomHeuristic derives from Heuristic to implement custom heuristics."""

    @override
    def set_goal(self, goal: GroundConjunctiveCondition) -> None:
        pass

    @override
    def evaluate(self, state: State) -> float:
        raise NotImplementedError


class CustomPruningStrategy(PruningStrategy):
    """CustomPruningStrategy derives from PruningStrategy to implement custom strategies to prune states in search.

    A prominent example is Iterative Width (IW) that prunes state in a breadth-first (BrFS) search
    based on a state novelty criteria.
    """

    @override
    def should_prune_state(self, state: State) -> bool:
        """Is checked for the initial state."""
        raise NotImplementedError

    @override
    def should_prune_successor_state(self, state: State, succ_state: State, is_new_succ_state: bool) -> bool:
        """Is checked for every generated state."""
        raise NotImplementedError


class CustomGoalStrategy(GoalStrategy):
    """CustomGoalStrategy derives from GoalStrategy to implement custom strategies to terminate search at a goal state.

    A prominent example is Serialized Iterative Width (SIW) that repeatedly calls IW
    to greedily achieve one goal atom at a time.
    """

    @override
    def is_static_goal_satisfied(self, task: Task) -> bool:
        """Is checked before running a search."""
        raise NotImplementedError

    @override
    def is_dynamic_goal_satisfied(self, seed_state: State, state: State) -> bool:
        """Is checked for every generated state."""
        raise NotImplementedError


class CustomEventHandler(EventHandler):
    """CustomEventHandler derives from EventHandler to react to search lifecycle events.

    Worker-local node events are intentionally C++-only. Common counters are returned
    with SearchResult instead of being maintained by an event handler.
    """

    @override
    def on_start_search(self, node: Node, f_value: float) -> None:
        pass

    @override
    def on_end_search(self, status: SearchStatus, statistics: Statistics) -> None:
        pass

    @override
    def on_solved(self, plan: Plan) -> None:
        pass


def main() -> None:
    """Run A* eager search with the goal count heuristic on the given task and print the plan."""
    arg_parser = argparse.ArgumentParser(description="A* Eager Search.")
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
    heuristic = GoalCountHeuristic(lifted_task)
    axiom_evaluator_factory = AxiomEvaluatorFactory()
    state_repository_factory = StateRepositoryFactory()
    successor_generator_factory = SuccessorGeneratorFactory()
    axiom_evaluator = axiom_evaluator_factory.create(lifted_task, execution_context)
    state_repository = state_repository_factory.create(lifted_task)
    successor_generator = successor_generator_factory.create(lifted_task, execution_context)

    options = Options()  # Search defaults to one worker; ExecutionContext controls lifted inner parallelism.
    options.event_handler = DefaultEventHandler(0)  # Prints search progress when verbosity is nonzero.
    options.goal_strategy = ConjunctiveGoalStrategy(
        lifted_task
    )  # Terminates the search when reaching a state that satisfies the task's goal.
    options.pruning_strategy = PruningStrategy()  # Never prunes

    search_result = find_solution(
        lifted_task,
        state_repository,
        axiom_evaluator,
        successor_generator,
        heuristic,
        options,
    )

    print("Search status:", search_result.status)
    print("Search statistics:", search_result.statistics)

    plan = search_result.plan

    if plan is not None:
        print(f"Found plan with length {plan.get_length()} and cost {plan.get_cost()}")
        print(plan)
    else:
        print("No solution was found.")


if __name__ == "__main__":
    main()
