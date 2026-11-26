# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from collections.abc import Sequence

import pytest
from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pyyggdrasil.execution import ExecutionContext

from pytyr.formalism import Parser
from pytyr.planning import Transition, ground, lifted

Summary = tuple[str, list[tuple[int, float]]]


def _summary(label: Transition, distributions: Sequence[Sequence[ground.OutcomeNode | lifted.OutcomeNode]]) -> Summary:
    """A successor's action binding and, per distribution, its size and probability sum."""
    sizes = sorted(
        (len(outcomes), float(sum(outcome.probability for outcome in outcomes))) for outcomes in distributions
    )
    return (str(label.binding), sizes)


def _lifted_task(name: str) -> lifted.Task:
    task_files = fetch_task(f"{name}/test-1.pddl")
    options = ParserOptions()
    return lifted.Task(Parser(str(task_files.domain_path), options).parse_task(str(task_files.task_path), options))


def _lifted_summaries(name: str) -> list[Summary]:
    task = _lifted_task(name)
    execution = ExecutionContext(1)
    repository = lifted.StateRepository(task)
    evaluator = lifted.AxiomEvaluator(task, execution)
    generator = lifted.SuccessorGenerator(task, execution)
    result: list[Summary] = []

    def collect(successor: tuple[Transition, Sequence[Sequence[lifted.OutcomeNode]]]) -> bool:
        result.append(_summary(*successor))
        return True

    generator.for_each_successor(generator.get_initial_node(repository, evaluator), repository, evaluator, collect)
    return sorted(result)


def _ground_summaries(name: str) -> list[Summary]:
    execution = ExecutionContext(1)
    task = _lifted_task(name).instantiate_ground_task(execution).task
    repository = ground.StateRepository(task)
    evaluator = ground.AxiomEvaluator(task, execution)
    generator = ground.SuccessorGenerator(task, execution)
    result: list[Summary] = []

    def collect(successor: tuple[Transition, Sequence[Sequence[ground.OutcomeNode]]]) -> bool:
        result.append(_summary(*successor))
        return True

    generator.for_each_successor(generator.get_initial_node(repository, evaluator), repository, evaluator, collect)
    return sorted(result)


@pytest.mark.parametrize("name", ["nondeterministic/tests/ipc2008-faults", "probabilistic/tests/ippc2004-bw-nc-pc-nr"])
def test_successors_are_distributions(name: str) -> None:
    """Successors are sets of distributions summing to 1, alike ground and lifted; some have several outcomes."""
    summaries = _lifted_summaries(name)
    assert summaries == _ground_summaries(name)
    for _, distributions in summaries:
        for size, total in distributions:
            assert size > 0
            assert total == pytest.approx(1.0)
    assert any(len(distributions) > 1 or distributions[0][0] > 1 for _, distributions in summaries)
