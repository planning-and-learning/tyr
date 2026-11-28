# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""The README's successor API, alike on one task.

The environments, the enumeration and its options, and the axiom step.
"""

from collections.abc import Sequence

import pytest
from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pyyggdrasil.execution import ExecutionContext

from pytyr import formalism
from pytyr.planning import lifted


def test_the_three_levels_agree() -> None:
    task_files = fetch_task("classical/tests/gripper/test-1.pddl")
    parser_options = ParserOptions()
    parser = formalism.Parser(str(task_files.domain_path), parser_options)
    task = lifted.Task(parser.parse_task(str(task_files.task_path), parser_options))
    execution_context = ExecutionContext(1)
    axiom_evaluator = lifted.AxiomEvaluator(task, execution_context)
    state_repository = lifted.StateRepository(task)
    successor_generator = lifted.SuccessorGenerator(task, execution_context)
    initial_node = lifted.get_initial_node(state_repository, axiom_evaluator)

    # Every step at once: kept, in the general environment each a classical action's one distribution of one outcome.
    general = lifted.general
    simple = general.get_successors(successor_generator, state_repository, axiom_evaluator, initial_node)
    simple_metrics = sorted(successor.distributions()[0].outcomes()[0].node().metric() for successor in simple)
    assert simple
    assert all(len(successor.distributions()) == 1 for successor in simple)

    # The steps apart: generated (borrowed, their states not closed under the axioms), then the lookup, the axiom step,
    # registering and the label interned, each a copy.
    kept: list[lifted.general.OwnedSuccessorBuilderView] = []

    def visit(transition: lifted.TransitionLabel, successor: lifted.general.SuccessorBuilderView) -> bool:
        found = general.find_outcomes(state_repository, successor)
        extended = general.extend_outcomes(successor_generator, axiom_evaluator, found)
        assert extended is not None
        registered = general.register_outcomes(state_repository, extended)
        kept.append(general.intern_label(state_repository, registered))
        assert successor.clone().label().kind() == successor.label().kind() == transition.kind
        return True

    assert general.for_each_successor(successor_generator, initial_node, visit)
    assert len(kept) == len(simple)

    # What a search uses: the deterministic environment, one successor state per transition, registered into a node.
    deterministic = lifted.deterministic
    deterministic.require(successor_generator)
    metrics: list[float] = []

    found_states: list[bool] = []

    def visit_state(_transition: lifted.TransitionLabel, successor_state: lifted.SuccessorState) -> bool:
        found_states.append(lifted.find_successor(state_repository, initial_node, successor_state))
        lifted.extend_successor(axiom_evaluator, initial_node, successor_state)
        node = lifted.register_successor(state_repository, initial_node, successor_state)
        metrics.append(node.metric())
        return True

    assert deterministic.for_each_successor(successor_generator, initial_node, visit_state)
    assert sorted(metrics) == simple_metrics
    # The lookup finds every state: the simple call above registered them.
    assert all(found_states)

    # Its options. accept prunes by label, before a successor is generated.
    metrics = []
    assert deterministic.for_each_successor(
        successor_generator, initial_node, visit_state, accept=lambda _binding: False
    )
    assert metrics == []

    # arrange sees every label and returns the positions to generate, in order: here the last transition only, whose
    # label it keeps (interned) to apply it again below.
    picked: list[formalism.RowActionLifted] = []

    def last_only(labels: Sequence[lifted.TransitionLabel]) -> list[int]:
        binding = labels[-1].binding
        assert binding is not None
        picked.append(lifted.intern_action_binding(successor_generator, binding))
        return [len(labels) - 1]

    assert deterministic.for_each_successor(successor_generator, initial_node, visit_state, arrange=last_only)
    assert len(metrics) == 1
    assert metrics[0] in simple_metrics

    # stop ends the enumeration before the first transition.
    assert not deterministic.for_each_successor(successor_generator, initial_node, visit_state, stop=lambda: True)
    assert len(metrics) == 1

    # One known transition, by its binding, without enumerating the node.
    successor_state = deterministic.generate_successor(successor_generator, initial_node, picked[0])
    assert lifted.find_successor(state_repository, initial_node, successor_state)
    lifted.extend_successor(axiom_evaluator, initial_node, successor_state)
    applied = lifted.register_successor(state_repository, initial_node, successor_state)
    assert applied.metric() == metrics[0]
    # Making the node consumed the successor state.
    with pytest.raises(ValueError, match="consumed"):
        lifted.register_successor(state_repository, initial_node, successor_state)

    # A node's record, kept apart from it: what a search takes as its start node.
    packed = lifted.packed_node(initial_node)
    assert packed.state().index() == initial_node.state().pack().index()
    assert packed.metric() == initial_node.metric()

    # Every step at once in the deterministic environment: labeled nodes, registered and their labels interned, or
    # (intern=False) their states closed under the axioms and nothing stored.
    interned = deterministic.get_successors(successor_generator, state_repository, axiom_evaluator, initial_node)
    assert sorted(labeled.node().metric() for labeled in interned) == simple_metrics
    built = deterministic.get_successors(
        successor_generator, state_repository, axiom_evaluator, initial_node, intern=False
    )
    assert sorted(labeled.node().metric() for labeled in built) == simple_metrics
    assert [str(labeled.node().state()) for labeled in built] == [str(labeled.node().state()) for labeled in interned]

    # The deterministic axiom step without interning.
    metrics = []

    def visit_extended(_transition: lifted.TransitionLabel, successor_state: lifted.SuccessorState) -> bool:
        lifted.extend_successor(axiom_evaluator, initial_node, successor_state)
        metrics.append(lifted.successor_node(initial_node, successor_state).metric())
        return True

    assert deterministic.for_each_successor(successor_generator, initial_node, visit_extended)
    assert sorted(metrics) == simple_metrics

    # Where nature acts, the axiom step without interning gives the same outcomes.
    def visit_both(_transition: lifted.TransitionLabel, successor: lifted.general.SuccessorBuilderView) -> bool:
        extended = general.extend_outcomes(successor_generator, axiom_evaluator, successor)
        assert extended is not None
        registered = general.register_outcomes(state_repository, extended)
        assert str(extended.distributions()[0].outcomes()[0].node().state()) == str(
            registered.distributions()[0].outcomes()[0].node().state()
        )
        return True

    assert general.for_each_successor(successor_generator, initial_node, visit_both)
    unstored = general.get_successors(
        successor_generator, state_repository, axiom_evaluator, initial_node, intern=False
    )
    assert len(unstored) == len(simple)

    # A classical task fits every environment: under an adversary or chance alone, one outcome per transition.
    for environment in (lifted.nondeterministic, lifted.probabilistic):
        environment.require(successor_generator)
        successors = environment.get_successors(successor_generator, state_repository, axiom_evaluator, initial_node)
        assert sorted(successor.outcomes()[0].node().metric() for successor in successors) == simple_metrics
        assert all(len(successor.outcomes()) == 1 for successor in successors)
