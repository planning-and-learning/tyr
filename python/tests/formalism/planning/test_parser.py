# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
from pathlib import Path

import pytest
from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pyyggdrasil.execution import ExecutionContext

from pytyr.formalism import (
    GroundPlanningTask,
    LiftedPlanningTask,
    Parser,
    PlanningDomain,
    TaskGround,
    TaskLifted,
)
from pytyr.planning import lifted

GRIPPER = fetch_task("classical/tests/gripper/test-1.pddl")


def _parse_gripper_task() -> tuple[Parser, LiftedPlanningTask]:
    domain_filepath = str(GRIPPER.domain_path)
    problem_filepath = str(GRIPPER.task_path)
    parser_options = ParserOptions()
    parser = Parser(domain_filepath, parser_options)

    return parser, parser.parse_task(problem_filepath, parser_options)


def test_pddl_parser() -> None:
    """Test parsing and translation of a PDDL domain and problem file."""
    parser, task = _parse_gripper_task()
    domain = parser.get_domain()

    assert isinstance(task, LiftedPlanningTask)
    assert isinstance(task.get_task(), TaskLifted)
    assert domain.get_domain() == task.get_domain().get_domain()
    assert domain.get_path() == GRIPPER.domain_path
    assert task.get_path() == GRIPPER.task_path


@pytest.mark.parametrize(
    ("domain_path", "task_path"),
    [(None, None), (Path("virtual/../domain.pddl"), Path("virtual/../problem.pddl"))],
)
def test_in_memory_paths_are_optional_and_preserved_through_grounding(
    domain_path: Path | None,
    task_path: Path | None,
) -> None:
    options = ParserOptions()
    parser = Parser(GRIPPER.domain_path.read_text(), domain_path, options)
    task = parser.parse_task(GRIPPER.task_path.read_text(), task_path, options)

    assert parser.get_domain().get_path() == domain_path
    assert task.get_domain().get_path() == domain_path
    assert task.get_path() == task_path
    lifted_task = lifted.Task(task)
    assert lifted_task.get_formalism_task().get_path() == task_path
    grounded = lifted_task.instantiate_ground_task(
        ExecutionContext(1),
        lifted.GroundTaskInstantiationOptions(),
    )
    assert grounded.status == lifted.GroundTaskInstantiationStatus.SUCCESS
    assert isinstance(grounded.task.get_formalism_task(), GroundPlanningTask)
    assert isinstance(grounded.task.get_task(), TaskGround)
    assert grounded.task.get_formalism_task().get_path() == task_path
    assert grounded.task.get_formalism_task().get_domain().get_path() == domain_path
    assert task.get_task().num_objects() == 6
    assert grounded.task.get_task().num_objects() == 6


def test_programmatic_wrappers_default_to_no_path() -> None:
    parser, parsed_task = _parse_gripper_task()
    parsed_domain = parser.get_domain()
    domain = PlanningDomain(
        parsed_domain.get_domain(),
        parsed_domain.get_repository(),
    )
    task = LiftedPlanningTask(
        parsed_task.get_task(),
        parsed_task.get_fdr_context(),
        parsed_task.get_repository(),
        domain,
    )
    assert domain.get_path() is None
    assert task.get_path() is None


def test_parser_view_accessors_keep_temporary_owners_alive() -> None:
    parser_options = ParserOptions()
    domain_filepath = str(GRIPPER.domain_path)
    problem_filepath = str(GRIPPER.task_path)

    domain = Parser(domain_filepath, parser_options).get_domain().get_domain()
    task = Parser(domain_filepath, parser_options).parse_task(problem_filepath, parser_options).get_task()

    gc.collect()

    assert domain.name() == "gripper-strips"
    assert task.name() == "gripper-2"
    assert [object_.name() for object_ in task.objects()] == ["ball1", "ball2", "left", "right"]


def test_nested_task_view_accessors_keep_temporary_parent_views_alive() -> None:
    parser_options = ParserOptions()
    domain_filepath = str(GRIPPER.domain_path)
    problem_filepath = str(GRIPPER.task_path)

    task = Parser(domain_filepath, parser_options).parse_task(problem_filepath, parser_options).get_task()
    domain = task.domain()
    goal = task.goal()
    del task

    gc.collect()

    assert domain.name() == "gripper-strips"
    positive_goal_facts = list(goal.positive_facts())
    assert len(positive_goal_facts) == 1
    goal_atom = positive_goal_facts[0].atom()
    assert goal_atom is not None
    assert goal_atom.predicate().name() == "at"


def test_parsed_domain_exposes_symbol_metadata() -> None:
    parser, _ = _parse_gripper_task()
    domain = parser.get_domain().get_domain()

    static_predicates = [
        (int(predicate.index()), predicate.name(), predicate.arity())
        for predicate in domain.static_predicates()
    ]
    fluent_predicates = [
        (int(predicate.index()), predicate.name(), predicate.arity())
        for predicate in domain.fluent_predicates()
    ]

    for _, _, arity in static_predicates + fluent_predicates:
        assert isinstance(arity, int)
    assert static_predicates == [
        (0, "ball", 1),
        (1, "gripper", 1),
        (2, "number", 1),
        (3, "object", 1),
        (4, "room", 1),
    ]
    # Order follows loki's canonical predicate order (sorted by rendered PDDL text since pypddl 1.0.17),
    # so "(at ?b ?r)" sorts before "(at-robby ?r)".
    assert fluent_predicates == [
        (0, "at", 2),
        (1, "at-robby", 1),
        (2, "carry", 2),
        (3, "free", 1),
    ]


def test_nested_action_view_accessors_keep_temporary_parent_views_alive() -> None:
    parser, _ = _parse_gripper_task()
    action = next(iter(parser.get_domain().get_domain().actions()))
    condition = action.condition()
    del action

    gc.collect()

    assert condition.arity() == 3
    static_literals = list(condition.static_literals())
    assert len(static_literals) == 6

    literal = static_literals[0]
    atom = literal.atom()
    del literal
    gc.collect()

    predicate = atom.predicate()
    del atom
    gc.collect()

    assert predicate.name() in {"ball", "gripper", "room"}


def test_parsed_task_exposes_domain_action_and_goal_views() -> None:
    parser, parsed_task = _parse_gripper_task()
    domain = parser.get_domain().get_domain()
    task = parsed_task.get_task()

    assert domain.name() == "gripper-strips"
    assert task.name() == "gripper-2"
    assert task.num_objects() == 6
    assert len(list(domain.constants())) == 2

    assert [object_.name() for object_ in task.objects()] == [
        "ball1",
        "ball2",
        "left",
        "right",
    ]
    static_atoms = list(task.static_atoms())
    fluent_atoms = list(task.fluent_atoms())

    assert len(static_atoms) == 12
    assert len(fluent_atoms) == 5
    for atom in [*static_atoms, *fluent_atoms]:
        assert atom.predicate().arity() == len(list(atom.row().resolve_objects()))
    assert list(task.static_fterm_values()) == []
    assert list(task.fluent_fterm_values()) == []
    assert list(task.axioms()) == []

    actions = {action.name(): action for action in domain.actions()}

    assert sorted(actions) == ["drop", "move", "pick"]

    expected_action_metadata = {
        "drop": (3, 3, 6, 2),
        "move": (2, 2, 4, 1),
        "pick": (3, 3, 6, 3),
    }

    for action_name, (
        original_arity,
        arity,
        num_static_literals,
        num_fluent_literals,
    ) in expected_action_metadata.items():
        action = actions[action_name]
        condition = action.condition()
        static_literals = list(condition.static_literals())
        fluent_literals = list(condition.fluent_literals())

        assert action.original_arity() == original_arity
        assert action.arity() == arity
        assert len(list(action.variables())) == arity
        assert len(list(action.effects())) == 1
        assert condition.arity() == arity
        assert len(list(condition.variables())) == arity
        assert len(static_literals) == num_static_literals
        assert len(fluent_literals) == num_fluent_literals
        for literal in [*static_literals, *fluent_literals]:
            atom = literal.atom()
            assert atom.predicate().arity() == len(list(atom.terms()))
        assert list(condition.derived_literals()) == []
        assert list(condition.numeric_constraints()) == []

    goal = task.goal()

    assert len(list(goal.positive_facts())) == 1
    assert list(goal.negative_facts()) == []
    assert list(goal.static_literals()) == []
    assert list(goal.derived_literals()) == []
    assert list(goal.numeric_constraints()) == []


def test_fdr_context_exposes_variables_and_fact_metadata() -> None:
    _, task = _parse_gripper_task()
    fdr_context = task.get_fdr_context()
    fluent_atoms = list(task.get_task().fluent_atoms())
    variables = list(fdr_context.get_variables())

    variable_atoms = [atom for variable in variables for atom in variable.atoms()]

    assert variables
    assert set(fluent_atoms) <= set(variable_atoms)
    assert all(variable.domain_size() >= len(list(variable.atoms())) for variable in variables)
    assert sum(variable.domain_size() - 1 for variable in variables) >= len(fluent_atoms)

    for atom in fluent_atoms:
        fact = fdr_context.get_fact(atom)

        assert fact.has_value()
        assert fact.atom() == atom
        assert fact.variable() in variables


def test_fdr_context_view_accessors_keep_temporary_owners_alive() -> None:
    _, task = _parse_gripper_task()
    fluent_atom = next(iter(task.get_task().fluent_atoms()))

    variables = task.get_fdr_context().get_variables()
    fact = task.get_fdr_context().get_fact(fluent_atom)

    gc.collect()

    variables = list(variables)
    assert variables
    assert fact.has_value()
    assert fact.variable() in variables


def test_views_from_independent_parsers_are_distinct() -> None:
    first_parser = Parser(
        str(GRIPPER.domain_path),
        ParserOptions(),
    )
    second_parser = Parser(
        str(GRIPPER.domain_path),
        ParserOptions(),
    )

    first_predicate = next(iter(first_parser.get_domain().get_domain().fluent_predicates()))
    second_predicate = next(iter(second_parser.get_domain().get_domain().fluent_predicates()))

    assert int(first_predicate.index()) == int(second_predicate.index())
    assert first_predicate.name() == second_predicate.name()
    assert first_predicate != second_predicate
