import pytest
from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pytyr.formalism.planning import (
    ExhaustiveObjective,
    GreedyCriterion,
    Parser,
    compute_all_mutex_groups,
    compute_invariant_order_exhaustive,
    compute_invariant_order_greedy,
    compute_mutex_groups_in_order,
    synthesize_invariants,
)
from pytyr.planning import lifted
from pyyggdrasil.execution import ExecutionContext

FERRY = fetch_task("classical/tests/ferry/test-1.pddl")


def _predicate_names(invariant):
    return {str(atom.get_predicate().get_name()) for atom in invariant.get_atoms()}


@pytest.fixture
def ferry():
    parser = Parser(str(FERRY.domain_path), ParserOptions())
    # Interned into the domain's repository, so before any task of the domain is parsed.
    invariants = synthesize_invariants(parser.get_domain())
    formalism_task = parser.parse_task(str(FERRY.task_path), ParserOptions())
    # The parser owns the repository the invariants are interned into.
    return parser, formalism_task, invariants


ALL_ORDERS = [
    lambda invariants: compute_invariant_order_greedy(invariants, GreedyCriterion.GroupSize),
    lambda invariants: compute_invariant_order_greedy(invariants, GreedyCriterion.TotalCoverage),
    lambda invariants: compute_invariant_order_exhaustive(invariants, ExhaustiveObjective.FewestVariables),
    lambda invariants: compute_invariant_order_exhaustive(invariants, ExhaustiveObjective.LargestVariables),
    lambda invariants: compute_invariant_order_exhaustive(invariants, ExhaustiveObjective.Coverage),
]


@pytest.mark.parametrize("compute_order", ALL_ORDERS)
def test_order_does_not_depend_on_input_order(ferry, compute_order):
    _, _, invariants = ferry

    assert compute_order(list(reversed(invariants))) == compute_order(invariants)


def test_ferry_orders(ferry):
    _, _, invariants = ferry

    def names(order):
        return [_predicate_names(i) for i in order]

    assert names(compute_invariant_order_greedy(invariants)) == [{"at", "on"}, {"at-ferry"}]
    assert compute_invariant_order_greedy(invariants) == compute_invariant_order_greedy(invariants, GreedyCriterion.GroupSize)
    assert names(compute_invariant_order_exhaustive(invariants)) == [{"at", "on"}, {"at-ferry"}]
    assert compute_invariant_order_exhaustive(invariants) == compute_invariant_order_exhaustive(
        invariants, ExhaustiveObjective.FewestVariables)
    assert names(compute_invariant_order_exhaustive(invariants, ExhaustiveObjective.Coverage)) == [
        {"empty-ferry", "on"},
        {"at", "on"},
        {"at-ferry"},
    ]


def _ground(formalism_task, invariant_order):
    options = lifted.GroundTaskInstantiationOptions()
    options.invariant_order = invariant_order
    result = lifted.Task(formalism_task).instantiate_ground_task(ExecutionContext(1), options)
    assert result.status == lifted.GroundTaskInstantiationStatus.SUCCESS
    # The result owns the ground task; the variables are views into it.
    return result


def _variable_predicates(variables):
    return sorted(sorted(str(atom.get_predicate().get_name()) for atom in variable.get_atoms()) for variable in variables)


def test_grounding_follows_the_order(ferry):
    _, formalism_task, invariants = ferry

    result = _ground(formalism_task, compute_invariant_order_exhaustive(invariants))

    assert _variable_predicates(result.task.get_fdr_context().get_variables()) == [
        ["at", "at", "on"], ["at", "at", "on"], ["at-ferry", "at-ferry"], ["empty-ferry"]]


def test_empty_order_gives_one_variable_per_atom(ferry):
    _, formalism_task, _ = ferry

    result = _ground(formalism_task, [])

    assert all(len(variable.get_atoms()) == 1 for variable in result.task.get_fdr_context().get_variables())


def test_options_stay_backward_compatible():
    options = lifted.GroundTaskInstantiationOptions()
    assert options.invariant_order is None
    assert options.disable_invariant_synthesis is False
    assert lifted.GroundTaskInstantiationOptions(disable_invariant_synthesis=True).disable_invariant_synthesis is True


def _atom_text(atom):
    return f"{atom.get_predicate().get_name()}({','.join(str(o.get_name()) for o in atom.get_objects())})"


def test_all_mutex_groups_are_the_proven_instances(ferry):
    parser, formalism_task, invariants = ferry
    result = lifted.Task(formalism_task).instantiate_ground_task(ExecutionContext(1), lifted.GroundTaskInstantiationOptions())
    ground = result.task.get_task()
    all_atoms = list(ground.get_fluent_atoms())
    initial_atoms = [fact.get_atom() for fact in ground.get_fluent_facts()]

    groups = compute_all_mutex_groups(initial_atoms, all_atoms, invariants)

    texts = sorted(sorted(_atom_text(atom) for atom in group.atoms) for group in groups)
    assert texts == [
        ["at(car1,loc1)", "at(car1,loc3)", "on(car1)"],
        ["at(car2,loc1)", "at(car2,loc3)", "on(car2)"],
        ["at-ferry(loc1)", "at-ferry(loc3)"],
        ["empty-ferry()", "on(car1)", "on(car2)"],
    ]
    assert sorted((sorted(_predicate_names(invariant)), invariant.get_num_rigid_variables()) for invariant in invariants) == [
        (["at", "on"], 1), (["at-ferry"], 0), (["empty-ferry", "on"], 0)]
    for group in groups:
        assert group.invariant in invariants
        assert len(group.objects) == group.invariant.get_num_rigid_variables()
        assert all(set(group.objects) <= set(atom.get_objects()) for atom in group.atoms)


def test_variables_trace_back_to_their_invariant(ferry):
    _, formalism_task, invariants = ferry
    # Coverage keeps the ferry contents invariant, which has no rigid variable.
    order = compute_invariant_order_exhaustive(invariants, ExhaustiveObjective.Coverage)
    result = _ground(formalism_task, order)
    ground = result.task.get_task()
    all_atoms = list(ground.get_fluent_atoms())
    initial_atoms = [fact.get_atom() for fact in ground.get_fluent_facts()]

    groups = compute_mutex_groups_in_order(initial_atoms, all_atoms, order)

    # The same partition as the grounder's variables, each group naming the invariant it instantiates.
    assert _variable_predicates(result.task.get_fdr_context().get_variables()) == sorted(
        sorted(str(atom.get_predicate().get_name()) for atom in group.atoms) for group in groups)
    contents = [group for group in groups if _predicate_names(group.invariant) == {"empty-ferry", "on"}]
    assert len(contents) == 1 and contents[0].objects == []


def test_all_mutex_groups_need_every_atom_in_index_order(ferry):
    parser, formalism_task, invariants = ferry
    result = lifted.Task(formalism_task).instantiate_ground_task(ExecutionContext(1), lifted.GroundTaskInstantiationOptions())
    all_atoms = list(result.task.get_task().get_fluent_atoms())

    with pytest.raises(ValueError):
        compute_all_mutex_groups([], all_atoms[1:], invariants)
