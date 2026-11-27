# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from types import ModuleType

import pytest
from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pyyggdrasil.execution import ExecutionContext

from pytyr.formalism import Parser
from pytyr.planning import ground, lifted

Summary = tuple[str, tuple[int, ...], list[tuple[int, float]]]


def _summary(successor: ground.SuccessorBuilderView | lifted.SuccessorBuilderView) -> Summary:
    """A successor's action binding (its action's name and objects) and, per distribution, its size and probability sum."""
    binding = successor.get_label().get_binding()
    sizes = sorted(
        (len(distribution.get_outcomes()), float(sum(outcome.get_probability() for outcome in distribution.get_outcomes())))
        for distribution in successor.get_distributions()
    )
    return (binding.get_relation().get_name(), tuple(binding.get_values()), sizes)


def _lifted_task(name: str) -> lifted.Task:
    task_files = fetch_task(f"{name}/test-1.pddl")
    options = ParserOptions()
    return lifted.Task(Parser(str(task_files.domain_path), options).parse_task(str(task_files.task_path), options))


def _summaries(module: ModuleType, task: ground.Task | lifted.Task) -> list[Summary]:
    """The summaries of the successors of the task's initial node; registering a successor's outcomes keeps it."""
    execution = ExecutionContext(1)
    repository = module.StateRepository(task)
    evaluator = module.AxiomEvaluator(task, execution)
    generator = module.SuccessorGenerator(task, execution)
    result: list[Summary] = []

    def collect(successor: ground.SuccessorBuilderView | lifted.SuccessorBuilderView) -> bool:
        summary = _summary(successor)
        assert _summary(module.register_outcomes(repository, successor)) == summary
        result.append(summary)
        return True

    generator.for_each_successor(module.get_initial_node(repository, evaluator), evaluator, collect)
    return sorted(result)


@pytest.mark.parametrize("name", ["nondeterministic/tests/ipc2008-faults", "probabilistic/tests/ippc2004-bw-nc-pc-nr"])
def test_successors_are_distributions(name: str) -> None:
    """Successors are sets of distributions summing to 1, alike ground and lifted; some have several outcomes."""
    task = _lifted_task(name)
    summaries = _summaries(lifted, task)
    assert summaries == _summaries(ground, task.instantiate_ground_task(ExecutionContext(1)).task)
    for _, _, distributions in summaries:
        for size, total in distributions:
            assert size > 0
            assert total == pytest.approx(1.0)
    assert any(len(distributions) > 1 or distributions[0][0] > 1 for _, _, distributions in summaries)
