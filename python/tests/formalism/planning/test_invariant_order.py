import pytest
from pypddl.formalism import ParserOptions
from pypddl_datasets import fetch_task
from pytyr.formalism.planning import (
    Parser,
    compute_invariant_order_exhaustive,
    compute_invariant_order_greedy,
    synthesize_invariants,
)
from pytyr.planning import lifted
from pyyggdrasil.execution import ExecutionContext

FERRY = fetch_task("classical/tests/ferry/test-1.pddl")


def _predicate_names(invariant):
    return {str(atom.predicate.get_name()) for atom in invariant.atoms}


@pytest.fixture
def ferry():
    parser = Parser(str(FERRY.domain_path), ParserOptions())
    formalism_task = parser.parse_task(str(FERRY.task_path), ParserOptions())
    invariants = synthesize_invariants(parser.get_domain().get_domain())
    # The parser owns the repository the invariants refer to.
    return parser, formalism_task, invariants


@pytest.mark.parametrize("compute_order", [compute_invariant_order_greedy, compute_invariant_order_exhaustive])
def test_order_does_not_depend_on_input_order(ferry, compute_order):
    _, _, invariants = ferry

    assert compute_order(list(reversed(invariants))) == compute_order(invariants)


def test_ferry_orders(ferry):
    _, _, invariants = ferry

    assert [_predicate_names(i) for i in compute_invariant_order_greedy(invariants)] == [{"at", "on"}, {"at-ferry"}]
    assert [_predicate_names(i) for i in compute_invariant_order_exhaustive(invariants)] == [
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

    assert _variable_predicates(result.task.get_fdr_context().get_variables()) == [["at", "at"], ["at", "at"], ["at-ferry", "at-ferry"], ["empty-ferry", "on", "on"]]


def test_empty_order_gives_one_variable_per_atom(ferry):
    _, formalism_task, _ = ferry

    result = _ground(formalism_task, [])

    assert all(len(variable.get_atoms()) == 1 for variable in result.task.get_fdr_context().get_variables())


def test_options_stay_backward_compatible():
    options = lifted.GroundTaskInstantiationOptions()
    assert options.invariant_order is None
    assert options.disable_invariant_synthesis is False
    assert lifted.GroundTaskInstantiationOptions(disable_invariant_synthesis=True).disable_invariant_synthesis is True
