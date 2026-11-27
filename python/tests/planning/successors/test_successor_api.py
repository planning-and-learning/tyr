# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""The README's successor API at its three levels (simple, expert, lowest), alike on one task."""

from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pyyggdrasil.execution import ExecutionContext

import pytyr.formalism as formalism
import pytyr.planning.lifted as lifted


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

    # Simple: kept, each a classical action's one distribution of one outcome.
    simple = lifted.get_successors(successor_generator, state_repository, axiom_evaluator, initial_node)
    simple_metrics = sorted(
        successor.get_distributions()[0].get_outcomes()[0].get_node().get_metric() for successor in simple
    )
    assert simple
    assert all(len(successor.get_distributions()) == 1 for successor in simple)

    # Expert: borrowed, kept as copies with their outcome states registered and their labels interned.
    kept = []

    def visit(successor: lifted.SuccessorBuilderView) -> bool:
        kept.append(lifted.intern_label(state_repository, lifted.register_outcomes(state_repository, successor)))
        assert successor.clone().get_label().get_kind() == successor.get_label().get_kind()
        return True

    assert lifted.for_each_successor(successor_generator, initial_node, axiom_evaluator, visit)
    assert len(kept) == len(simple)

    # Lowest: one successor state per applicable binding, registered into a node.
    metrics = []

    def visit_binding(binding: formalism.RowActionLiftedBuilderView) -> bool:
        successor_state = lifted.generate_successor_state(successor_generator, state_repository, initial_node, binding)
        node = lifted.register_successor(state_repository, axiom_evaluator, initial_node, successor_state)
        metrics.append(node.get_metric())
        return True

    assert lifted.for_each_applicable_action_binding(successor_generator, initial_node, visit_binding)
    assert sorted(metrics) == simple_metrics
