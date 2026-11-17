# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""A planning task prints as PDDL: loki parses the text, and tyr parses it back to the same task."""

import pytest
from pypddl import formalism as loki
from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task

from pytyr.formalism.planning import LiftedPlanningTask, Parser

# Typing, conditional effects, derived predicates (philosophers, psr-middle), action costs, numeric fluents.
CASES = [
    "classical/tests/gripper/test-1.pddl",
    "classical/tests/miconic-fulladl/test-1.pddl",
    "classical/tests/philosophers/test-1.pddl",
    "classical/tests/psr-middle/test-1.pddl",
    "classical/tests/parcprinter/test-1.pddl",
    "numeric/tests/zenotravel/test-1.pddl",
]


def _parse(name: str) -> LiftedPlanningTask:
    source = fetch_task(name)
    options = ParserOptions()
    return Parser(str(source.domain_path), options).parse_task(str(source.task_path), options)


def _domain_and_problem(text: str) -> tuple[str, str]:
    """A planning task's text: its domain, then its problem."""
    position = text.index("\n(define (problem")
    return text[:position], text[position + 1 :]


@pytest.mark.parametrize("name", CASES)
def test_planning_task_prints_as_pddl_that_loki_parses(name: str) -> None:
    domain_text, problem_text = _domain_and_problem(str(_parse(name)))

    assert domain_text.startswith("(define (domain ")
    assert problem_text.startswith("(define (problem ")
    parser = loki.Parser(domain_text, ParserOptions())
    parser.parse_task(problem_text)


@pytest.mark.parametrize("name", CASES)
def test_planning_task_text_parses_back_to_the_same_task(name: str) -> None:
    task = _parse(name)
    domain_text, problem_text = _domain_and_problem(str(task))
    options = ParserOptions()
    reparsed = Parser(domain_text, None, options).parse_task(problem_text, None, options)

    original, again = task.get_task(), reparsed.get_task()
    assert len(again.get_domain().get_actions()) == len(original.get_domain().get_actions())
    assert len(again.get_domain().get_axioms()) == len(original.get_domain().get_axioms())
    for predicates in ("get_static_predicates", "get_fluent_predicates", "get_derived_predicates"):
        assert len(getattr(again.get_domain(), predicates)()) == len(getattr(original.get_domain(), predicates)())
    assert len(again.get_objects()) == len(original.get_objects())
    assert len(again.get_static_atoms()) == len(original.get_static_atoms())
    assert len(again.get_fluent_atoms()) == len(original.get_fluent_atoms())
    # Printing what was read from the text gives the text again.
    assert str(reparsed) == str(task)
