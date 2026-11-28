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


def _summary(successor: ground.general.SuccessorBuilderView | lifted.general.SuccessorBuilderView) -> Summary:
    """A successor's action binding (its action's name and objects) and, per distribution, its size and probability sum."""
    binding = successor.label().binding()
    sizes = sorted(
        (len(distribution.outcomes()), float(sum(outcome.probability() for outcome in distribution.outcomes())))
        for distribution in successor.distributions()
    )
    return (binding.relation().name(), tuple(binding.values()), sizes)


def _lifted_task(name: str) -> lifted.Task:
    task_files = fetch_task(f"{name}/test-1.pddl")
    options = ParserOptions()
    return lifted.Task(Parser(str(task_files.domain_path), options).parse_task(str(task_files.task_path), options))


def _summaries(module: ModuleType, task: ground.Task | lifted.Task) -> list[Summary]:
    """The summaries of the successors of the task's initial node; the three steps keep a successor."""
    execution = ExecutionContext(1)
    repository = module.StateRepository(task)
    evaluator = module.AxiomEvaluator(task, execution)
    generator = module.SuccessorGenerator(task, execution)
    result: list[Summary] = []

    def collect(
        transition: ground.TransitionLabel | lifted.TransitionLabel,
        successor: ground.general.SuccessorBuilderView | lifted.general.SuccessorBuilderView,
    ) -> bool:
        summary = _summary(successor)
        found = module.general.find_outcomes(repository, successor)
        extended = module.general.extend_outcomes(generator, evaluator, found)
        assert extended is not None
        assert _summary(module.general.register_outcomes(repository, extended)) == summary
        result.append(summary)
        return True

    module.general.for_each_successor(generator, module.get_initial_node(repository, evaluator), collect)
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


def _environment_outcomes(environment: ModuleType, module: ModuleType, task: ground.Task | lifted.Task) -> list[int]:
    """The number of outcomes of each successor of the task's initial node in an environment whose successors hold them."""
    execution = ExecutionContext(1)
    repository = module.StateRepository(task)
    evaluator = module.AxiomEvaluator(task, execution)
    generator = module.SuccessorGenerator(task, execution)
    environment.require(generator)
    successors = environment.get_successors(generator, repository, evaluator, module.get_initial_node(repository, evaluator))
    return sorted(len(successor.outcomes()) for successor in successors)


def test_environments_follow_a_tasks_effects() -> None:
    """A oneof task has an adversary's outcomes, a probabilistic one chance's; neither is deterministic."""
    oneof = _lifted_task("nondeterministic/tests/ipc2008-faults")
    probabilistic = _lifted_task("probabilistic/tests/ippc2004-bw-nc-pc-nr")
    execution = ExecutionContext(1)

    features = lifted.SuccessorGenerator(oneof, execution).features()
    assert (features.oneof, features.probabilistic, features.temporal) == (True, False, False)
    features = lifted.SuccessorGenerator(probabilistic, execution).features()
    assert (features.oneof, features.probabilistic, features.temporal) == (False, True, False)

    for task, fitting, other in (
        (oneof, lifted.nondeterministic, lifted.probabilistic),
        (probabilistic, lifted.probabilistic, lifted.nondeterministic),
    ):
        generator = lifted.SuccessorGenerator(task, execution)
        fitting.require(generator)
        lifted.general.require(generator)
        for unfit in (lifted.deterministic, other):
            with pytest.raises(ValueError, match="environment"):
                unfit.require(generator)

    # An adversary's outcomes: several where a oneof fires, alike ground and lifted.
    outcomes = _environment_outcomes(lifted.nondeterministic, lifted, oneof)
    assert outcomes == _environment_outcomes(
        ground.nondeterministic, ground, oneof.instantiate_ground_task(ExecutionContext(1)).task
    )
    assert outcomes and outcomes[-1] > 1

    # Chance's outcomes: their probabilities sum to 1.
    repository = lifted.StateRepository(probabilistic)
    evaluator = lifted.AxiomEvaluator(probabilistic, execution)
    generator = lifted.SuccessorGenerator(probabilistic, execution)
    successors = lifted.probabilistic.get_successors(
        generator, repository, evaluator, lifted.get_initial_node(repository, evaluator)
    )
    assert successors
    for successor in successors:
        assert sum(outcome.probability() for outcome in successor.outcomes()) == pytest.approx(1.0)
    assert any(len(successor.outcomes()) > 1 for successor in successors)
