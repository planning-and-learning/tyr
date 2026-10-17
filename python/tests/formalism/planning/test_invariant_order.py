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


def test_grounding_follows_the_order(ferry):
    _, formalism_task, invariants = ferry
    order = compute_invariant_order_exhaustive(invariants)

    result = lifted.Task(formalism_task).instantiate_ground_task(
        ExecutionContext(1), lifted.GroundTaskInstantiationOptions(invariant_order=order)
    )

    assert result.status == lifted.GroundTaskInstantiationStatus.SUCCESS
    mutex_groups = result.mutex_groups
    assert mutex_groups.invariants == order
    variables = result.task.get_fdr_context().get_variables()
    assert len(variables) == len(mutex_groups.groups)
    contents = [g for g in mutex_groups.groups if g.invariant_index == 0]
    assert len(contents) == 1
    assert sorted(str(atom.get_predicate().get_name()) for atom in contents[0].atoms) == ["empty-ferry", "on", "on"]
    assert mutex_groups.fallbacks == []


def test_empty_order_gives_one_variable_per_atom(ferry):
    _, formalism_task, _ = ferry

    result = lifted.Task(formalism_task).instantiate_ground_task(
        ExecutionContext(1), lifted.GroundTaskInstantiationOptions(invariant_order=[])
    )

    assert all(len(group.atoms) == 1 and group.invariant_index is None for group in result.mutex_groups.groups)


def test_default_options_use_the_greedy_cover():
    assert lifted.GroundTaskInstantiationOptions().invariant_order is None
