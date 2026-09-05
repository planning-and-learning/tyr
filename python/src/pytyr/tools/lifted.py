from __future__ import annotations

from datetime import timedelta

from pyyggdrasil.execution import ExecutionContext
from pytyr.planning import SearchBudget
from pytyr.planning.lifted import (
    AxiomEvaluator,
    FFRPGHeuristic,
    SearchResult,
    StateRepository,
    SuccessorGenerator,
    Task,
    gbfs_lazy,
)


def find_satisficing_plan(
    task: Task,
    state_repository: StateRepository,
    axiom_evaluator: AxiomEvaluator,
    successor_generator: SuccessorGenerator,
    execution_context: ExecutionContext,
    search_budget: SearchBudget = SearchBudget(1_000_000, timedelta(seconds=60)),
) -> SearchResult:
    options = gbfs_lazy.Options()
    options.search_budget = search_budget
    return gbfs_lazy.find_solution(
        task,
        state_repository,
        axiom_evaluator,
        successor_generator,
        FFRPGHeuristic(task, execution_context),
        options,
    )
