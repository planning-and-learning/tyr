from pathlib import Path

import pytest
from pypddl.formalism import ParserOptions
from pytyr.formalism.planning import Parser, PlanningDomain, PlanningTask
from pytyr.planning import lifted
from pyyggdrasil.execution import ExecutionContext

from pypddl_datasets import fetch_task

GRIPPER = fetch_task("classical/tests/gripper/test-1.pddl")


def _parse_gripper_task():
    domain_filepath = str(GRIPPER.domain_path)
    problem_filepath = str(GRIPPER.task_path)
    parser_options = ParserOptions()
    parser = Parser(domain_filepath, parser_options)

    return parser, parser.parse_task(problem_filepath, parser_options)


def test_pddl_parser():
    """Test parsing and translation of a PDDL domain and problem file."""
    parser, task = _parse_gripper_task()
    domain = parser.get_domain()

    assert domain.get_domain() == task.get_domain().get_domain()
    assert domain.get_path() == GRIPPER.domain_path
    assert task.get_path() == GRIPPER.task_path


@pytest.mark.parametrize(
    ("domain_path", "task_path"),
    [(None, None), (Path("virtual/../domain.pddl"), Path("virtual/../problem.pddl"))],
)
def test_in_memory_paths_are_optional_and_preserved_through_grounding(
    domain_path: Path | None, task_path: Path | None,
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
        ExecutionContext(1), lifted.GroundTaskInstantiationOptions(),
    )
    assert grounded.status == lifted.GroundTaskInstantiationStatus.SUCCESS
    assert grounded.task.get_formalism_task().get_path() == task_path
    assert grounded.task.get_formalism_task().get_domain().get_path() == domain_path


def test_programmatic_wrappers_default_to_no_path() -> None:
    parser, parsed_task = _parse_gripper_task()
    parsed_domain = parser.get_domain()
    domain = PlanningDomain(
        parsed_domain.get_domain(), parsed_domain.get_repository(),
        parsed_domain.get_repository_factory(),
    )
    task = PlanningTask(
        parsed_task.get_task(), parsed_task.get_fdr_context(),
        parsed_task.get_repository(), domain,
    )
    assert domain.get_path() is None
    assert task.get_path() is None


def test_parser_view_accessors_keep_temporary_owners_alive():
    import gc

    parser_options = ParserOptions()
    domain_filepath = str(GRIPPER.domain_path)
    problem_filepath = str(GRIPPER.task_path)

    domain = Parser(domain_filepath, parser_options).get_domain().get_domain()
    task = Parser(domain_filepath, parser_options).parse_task(problem_filepath, parser_options).get_task()

    gc.collect()

    assert domain.get_name() == "gripper-strips"
    assert task.get_name() == "gripper-2"
    assert [object_.get_name() for object_ in task.get_objects()] == ["ball1", "ball2", "left", "right"]


def test_nested_task_view_accessors_keep_temporary_parent_views_alive():
    import gc

    parser_options = ParserOptions()
    domain_filepath = str(GRIPPER.domain_path)
    problem_filepath = str(GRIPPER.task_path)

    task = Parser(domain_filepath, parser_options).parse_task(problem_filepath, parser_options).get_task()
    domain = task.get_domain()
    goal = task.get_goal()
    del task

    gc.collect()

    assert domain.get_name() == "gripper-strips"
    positive_goal_facts = list(goal.get_positive_facts())
    assert len(positive_goal_facts) == 1
    goal_atom = positive_goal_facts[0].get_atom()
    assert goal_atom is not None
    assert goal_atom.get_predicate().get_name() == "at"


def test_parsed_domain_exposes_symbol_metadata():
    parser, _ = _parse_gripper_task()
    domain = parser.get_domain().get_domain()

    static_predicates = [
        (int(predicate.get_index()), predicate.get_name(), predicate.get_arity())
        for predicate in domain.get_static_predicates()
    ]
    fluent_predicates = [
        (int(predicate.get_index()), predicate.get_name(), predicate.get_arity())
        for predicate in domain.get_fluent_predicates()
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


def test_nested_action_view_accessors_keep_temporary_parent_views_alive():
    import gc

    parser, _ = _parse_gripper_task()
    action = next(iter(parser.get_domain().get_domain().get_actions()))
    condition = action.get_condition()
    del action

    gc.collect()

    assert condition.get_arity() == 3
    static_literals = list(condition.get_static_literals())
    assert len(static_literals) == 6

    literal = static_literals[0]
    atom = literal.get_atom()
    del literal
    gc.collect()

    predicate = atom.get_predicate()
    del atom
    gc.collect()

    assert predicate.get_name() in {"ball", "gripper", "room"}


def test_parsed_task_exposes_domain_action_and_goal_views():
    parser, parsed_task = _parse_gripper_task()
    domain = parser.get_domain().get_domain()
    task = parsed_task.get_task()

    assert domain.get_name() == "gripper-strips"
    assert task.get_name() == "gripper-2"

    assert [object_.get_name() for object_ in task.get_objects()] == [
        "ball1",
        "ball2",
        "left",
        "right",
    ]
    static_atoms = list(task.get_static_atoms())
    fluent_atoms = list(task.get_fluent_atoms())

    assert len(static_atoms) == 12
    assert len(fluent_atoms) == 5
    for atom in [*static_atoms, *fluent_atoms]:
        assert atom.get_predicate().get_arity() == len(list(atom.get_objects()))
    assert list(task.get_static_fterm_values()) == []
    assert list(task.get_fluent_fterm_values()) == []
    assert list(task.get_axioms()) == []

    actions = {action.get_name(): action for action in domain.get_actions()}

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
        condition = action.get_condition()
        static_literals = list(condition.get_static_literals())
        fluent_literals = list(condition.get_fluent_literals())

        assert action.get_original_arity() == original_arity
        assert action.get_arity() == arity
        assert len(list(action.get_variables())) == arity
        assert len(list(action.get_effects())) == 1
        assert condition.get_arity() == arity
        assert len(list(condition.get_variables())) == arity
        assert len(static_literals) == num_static_literals
        assert len(fluent_literals) == num_fluent_literals
        for literal in [*static_literals, *fluent_literals]:
            atom = literal.get_atom()
            assert atom.get_predicate().get_arity() == len(list(atom.get_terms()))
        assert list(condition.get_derived_literals()) == []
        assert list(condition.get_numeric_constraints()) == []

    goal = task.get_goal()

    assert len(list(goal.get_positive_facts())) == 1
    assert list(goal.get_negative_facts()) == []
    assert list(goal.get_static_facts()) == []
    assert list(goal.get_derived_facts()) == []
    assert list(goal.get_numeric_constraints()) == []


def test_fdr_context_exposes_variables_and_fact_metadata():
    _, task = _parse_gripper_task()
    fdr_context = task.get_fdr_context()
    fluent_atoms = list(task.get_task().get_fluent_atoms())
    variables = list(fdr_context.get_variables())

    variable_atoms = [atom for variable in variables for atom in variable.get_atoms()]

    assert variables
    assert set(fluent_atoms) <= set(variable_atoms)
    assert all(variable.get_domain_size() >= len(list(variable.get_atoms())) for variable in variables)
    assert sum(variable.get_domain_size() - 1 for variable in variables) >= len(fluent_atoms)

    for atom in fluent_atoms:
        fact = fdr_context.get_fact(atom)

        assert fact.has_value()
        assert fact.get_atom() == atom
        assert fact.get_variable() in variables


def test_fdr_context_view_accessors_keep_temporary_owners_alive():
    import gc

    _, task = _parse_gripper_task()
    fluent_atom = next(iter(task.get_task().get_fluent_atoms()))

    variables = task.get_fdr_context().get_variables()
    fact = task.get_fdr_context().get_fact(fluent_atom)

    gc.collect()

    variables = list(variables)
    assert variables
    assert fact.has_value()
    assert fact.get_variable() in variables


def test_views_from_independent_parsers_use_deterministic_factory_local_identity():
    first_parser = Parser(
        str(GRIPPER.domain_path),
        ParserOptions(),
    )
    second_parser = Parser(
        str(GRIPPER.domain_path),
        ParserOptions(),
    )

    first_predicate = next(iter(first_parser.get_domain().get_domain().get_fluent_predicates()))
    second_predicate = next(iter(second_parser.get_domain().get_domain().get_fluent_predicates()))

    assert int(first_predicate.get_index()) == int(second_predicate.get_index())
    assert first_predicate.get_name() == second_predicate.get_name()
    assert first_predicate == second_predicate
    assert hash(first_predicate) == hash(second_predicate)
