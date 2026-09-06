import gc
import json
import sys
from collections.abc import Iterator
from typing import Literal

import pytest
from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext
from pytyr.formalism import planning as fp
from pytyr.planning import SearchStatus, ground, lifted
from pytyr.serialization import Dictionaries


DOMAIN = """(define (domain serialize)
  (:requirements :strips :fluents :derived-predicates)
  (:predicates (ready) (start) (done) (active))
  (:functions (capacity) (fuel))
  (:derived (active) (start))
  (:action finish :parameters ()
    :precondition (and (ready) (active) (> (capacity) 0))
    :effect (and (not (start)) (done) (decrease (fuel) 1))))
"""
PROBLEM = """(define (problem serialize-1) (:domain serialize)
  (:init (ready) (start) (= (capacity) 10) (= (fuel) 3))
  (:goal (done)))
"""


@pytest.mark.parametrize("backend", ["ground", "lifted"])
def test_native_plan_tables_and_lifetime(backend: Literal["ground", "lifted"]) -> None:
    parser = fp.Parser(DOMAIN, None, ParserOptions())
    task = lifted.Task(parser.parse_task(PROBLEM, None, ParserOptions()))
    execution = ExecutionContext(1)
    if backend == "ground":
        instantiated = task.instantiate_ground_task(execution, lifted.GroundTaskInstantiationOptions())
        assert instantiated.status == lifted.GroundTaskInstantiationStatus.SUCCESS
        task = instantiated.task
        repository = ground.StateRepositoryFactory().create(task)
        evaluator = ground.AxiomEvaluatorFactory().create(task, execution)
        generator = ground.SuccessorGeneratorFactory().create(task, execution)
        result = ground.brfs.find_solution(task, repository, evaluator, generator, ground.brfs.Options())
        state_type = ground.State
        node_type = ground.Node
    else:
        repository = lifted.StateRepositoryFactory().create(task)
        evaluator = lifted.AxiomEvaluatorFactory().create(task, execution)
        generator = lifted.SuccessorGeneratorFactory().create(task, execution)
        result = lifted.brfs.find_solution(task, repository, evaluator, generator, lifted.brfs.Options())
        state_type = lifted.State
        node_type = lifted.Node
    assert result.status == SearchStatus.SOLVED
    plan = result.plan
    assert plan is not None

    dictionaries = Dictionaries()
    dictionaries.register_table(state_type, "visited", "s")
    dictionaries.register_table(node_type, "nodes", "v")
    dictionaries.register_table(fp.ActionBinding, "actions", "a")
    dictionaries.register_table(fp.FluentGroundFunctionTerm, "functions", "n")
    dictionaries.register_table(fp.FluentGroundAtom, "atoms", "p")
    dictionaries.register_table(fp.DerivedGroundAtom, "derived", "d")
    references = sys.getrefcount(plan)
    data = dictionaries.serialize(plan)
    assert sys.getrefcount(plan) > references
    assert data == str(plan)
    assert dictionaries.table(state_type) == []
    assert dictionaries.serialize(plan.get_start_node()) == "v0"
    step, = plan.get_labeled_succ_nodes()
    assert [atom.get_predicate().get_name() for atom in plan.get_start_node().get_state().fluent_atoms()] == ["start"]
    assert [atom.get_predicate().get_name() for atom in step.node.get_state().fluent_atoms()] == ["done"]
    assert dictionaries.serialize(step) == str(step)
    assert dictionaries.serialize(step.node) == "v1"
    assert dictionaries.serialize(step.label) == "a0"
    assert dictionaries.table(fp.ActionBinding) == [{"relation": str(step.label.get_relation()), "objects": []}]
    assert [row["state"] for row in dictionaries.table(node_type)] == ["s0", "s1"]
    assert dictionaries.serialize(plan) == data
    states = dictionaries.table(state_type)
    assert len(states) == 2
    assert states[0]["fluent_ground_atoms"] == ["p0"]
    assert states[1]["fluent_ground_atoms"] == ["p1"]
    assert states[0]["fluent_ground_function_term_values"] == [["n0", 3.0]]
    assert states[1]["fluent_ground_function_term_values"] == [["n0", 2.0]]
    assert states[0]["derived_ground_atoms"] == ["d0"]
    assert states[1]["derived_ground_atoms"] == []
    tables = dictionaries.tables()
    assert tables["visited"] == {"prefix": "s", "rows": states}
    encoded_tables = json.dumps(tables)
    assert json.loads(encoded_tables) == tables
    snapshot = dictionaries.tables()
    snapshot["visited"]["rows"][0]["annotation"] = {"selected": True}
    assert "annotation" not in dictionaries.tables()["visited"]["rows"][0]
    states.clear()
    assert len(dictionaries.table(state_type)) == 2
    del plan, result, generator, evaluator, repository, task, parser
    gc.collect()
    assert dictionaries.tables() == tables


def test_registration_errors_and_native_text() -> None:
    parser = fp.Parser(DOMAIN, None, ParserOptions())
    task = lifted.Task(parser.parse_task(PROBLEM, None, ParserOptions()))
    term = next(iter(task.get_task().get_static_fterm_values())).get_fterm()
    dictionaries = Dictionaries()
    with pytest.raises(TypeError):
        dictionaries.register_table(str, "strings", "s")  # pyright: ignore[reportArgumentType]
    with pytest.raises(ValueError):
        dictionaries.register_table(fp.StaticGroundFunctionTerm, "terms", "t0")
    dictionaries.register_table(fp.StaticGroundFunctionTerm, "terms", "t")
    with pytest.raises(ValueError):
        dictionaries.register_table(fp.StaticGroundFunctionTerm, "more_terms", "u")
    assert dictionaries.serialize(term) == "t0"
    with pytest.raises(RuntimeError):
        dictionaries.register_table(fp.FluentGroundFunctionTerm, "fluent_terms", "f")
    with pytest.raises(ValueError):
        dictionaries.table(fp.FluentGroundFunctionTerm)
    with pytest.raises(TypeError):
        dictionaries.serialize("a native value is required")  # pyright: ignore[reportCallIssue, reportArgumentType]

    inline = Dictionaries()
    inline.register_table(fp.StaticFunctionBinding, "bindings", "b")
    assert inline.serialize(term) == str(term)
    assert inline.table(fp.StaticFunctionBinding) == []
    owner = inline.serialize(task.get_formalism_task())
    assert owner == str(task.get_formalism_task())
    assert json.loads(json.dumps(owner)) == owner
    assert inline.serialize(task.get_formalism_task()) == owner


def test_registered_variant_preserves_numeric_constant() -> None:
    parser = fp.Parser(DOMAIN, None, ParserOptions())
    repository = parser.get_domain().get_repository()
    expression = repository.create(fp.FunctionExpressionData(3.5))
    dictionaries = Dictionaries()
    dictionaries.register_table(fp.FunctionExpression, "expressions", "e")
    assert dictionaries.serialize(expression) == "e0"
    expected = [{"kind": "constant", "value": 3.5}]
    assert dictionaries.table(fp.FunctionExpression) == expected
    snapshot = dictionaries.tables()
    snapshot["expressions"]["rows"][0]["value"] = 8
    assert dictionaries.serialize(expression) == "e0"
    assert dictionaries.table(fp.FunctionExpression) == expected


@pytest.mark.parametrize("fields", [None, [], ["static_ground_atoms", "name"]])
def test_task_field_selection_limits_collected_descendants(fields: list[str] | None) -> None:
    parser = fp.Parser(DOMAIN, None, ParserOptions())
    task = parser.parse_task(PROBLEM, None, ParserOptions())
    dictionaries = Dictionaries()
    dictionaries.register_table(fp.LiftedTask, "tasks", "t", fields=fields)
    dictionaries.register_table(fp.StaticGroundAtom, "static_atoms", "a")
    dictionaries.register_table(fp.StaticPredicateBinding, "bindings", "b")
    dictionaries.register_table(fp.StaticPredicate, "predicates", "p")
    dictionaries.register_table(fp.FluentGroundAtom, "fluent_atoms", "f")

    assert dictionaries.serialize(task.get_task()) == "t0"
    row, = dictionaries.table(fp.LiftedTask)
    if fields == []:
        assert row == {}
        assert all(not snapshot["rows"] for name, snapshot in dictionaries.tables().items() if name != "tasks")
    else:
        assert row["name"] == "serialize-1"
        assert row["static_ground_atoms"] == ["a0"]
        assert dictionaries.table(fp.StaticGroundAtom) == [{"binding": "b0"}]
        assert dictionaries.table(fp.StaticPredicateBinding) == [{"relation": "p0", "objects": []}]
        assert [predicate["name"] for predicate in dictionaries.table(fp.StaticPredicate)] == ["ready"]
        if fields is None:
            assert row["fluent_ground_atoms"] == ["f0"]
            assert len(dictionaries.table(fp.FluentGroundAtom)) == 1
        else:
            assert list(row) == ["name", "static_ground_atoms"]
            assert dictionaries.table(fp.FluentGroundAtom) == []


def test_registered_variant_kind_does_not_collect_omitted_value() -> None:
    parser = fp.Parser(DOMAIN, None, ParserOptions())
    task = parser.parse_task(PROBLEM, None, ParserOptions())
    term = next(iter(task.get_task().get_static_fterm_values())).get_fterm()
    expression = task.get_repository().create(fp.GroundFunctionExpressionData(term))
    dictionaries = Dictionaries()
    dictionaries.register_table(fp.GroundFunctionExpression, "expressions", "e", fields=("kind",))
    dictionaries.register_table(fp.StaticGroundFunctionTerm, "terms", "t")

    assert dictionaries.serialize(expression) == "e0"
    assert dictionaries.table(fp.GroundFunctionExpression) == [{"kind": "StaticGroundFunctionTerm"}]
    assert dictionaries.table(fp.StaticGroundFunctionTerm) == []


@pytest.mark.parametrize("fields", [None, ["name"]])
def test_projected_binding_collects_only_selected_output_fields(fields: list[str] | None) -> None:
    parser = fp.Parser("""(define (domain bindings)
      (:requirements :strips)
      (:predicates (ready ?item) (done ?item))
      (:action finish :parameters (?item) :precondition (ready ?item)
        :effect (and (not (ready ?item)) (done ?item))))""", None, ParserOptions())
    task = parser.parse_task("""(define (problem binding-1) (:domain bindings)
      (:objects item) (:init (ready item)) (:goal (done item)))""", None, ParserOptions())
    action, = parser.get_domain().get_domain().get_actions()
    item, = task.get_task().get_objects()
    binding = task.get_repository().get_or_create(fp.ActionBindingData(action, [item]))
    calls: list[fp.ActionBinding] = []

    def project(value: fp.ActionBinding) -> dict[str, object]:
        calls.append(value)
        return {"name": value.get_relation().get_name(), "objects": value.get_objects()}

    dictionaries = Dictionaries()
    dictionaries.register_table(fp.ActionBinding, "bindings", "b", fields=fields, project=project)
    dictionaries.register_table(fp.Object, "objects", "o")
    dictionaries.register_table(fp.Action, "actions", "a")
    assert dictionaries.serialize(binding) == "b0"
    assert dictionaries.serialize(binding) == "b0"
    assert calls == [binding]
    assert dictionaries.table(fp.Action) == []
    if fields is None:
        assert dictionaries.table(fp.ActionBinding) == [{"name": "finish", "objects": ["o0"]}]
        assert [row["name"] for row in dictionaries.table(fp.Object)] == ["item"]
    else:
        assert dictionaries.table(fp.ActionBinding) == [{"name": "finish"}]
        assert dictionaries.table(fp.Object) == []


@pytest.mark.parametrize("bad_value", [[], {"unknown": object()}])
def test_invalid_projection_invalidates_registry(bad_value: object) -> None:
    parser = fp.Parser(DOMAIN, None, ParserOptions())
    expression = parser.get_domain().get_repository().create(fp.FunctionExpressionData(3.5))
    dictionaries = Dictionaries()
    dictionaries.register_table(
        fp.FunctionExpression, "expressions", "e",
        project=lambda _value: bad_value,  # pyright: ignore[reportArgumentType]
    )
    with pytest.raises(TypeError):
        dictionaries.serialize(expression)
    with pytest.raises(RuntimeError):
        dictionaries.tables()


@pytest.mark.parametrize("value,expected", [
    (fp.ArithmeticOperatorKind.Add, "+"),
    (fp.BooleanOperatorKind.Ge, ">="),
    (fp.NumericEffectOperatorKind.Increase, "increase"),
    (fp.OptimizationDirection.Minimize, "minimize"),
])
def test_projection_uses_native_enum_text(value: object, expected: str) -> None:
    parser = fp.Parser(DOMAIN, None, ParserOptions())
    expression = parser.get_domain().get_repository().create(fp.FunctionExpressionData(3.5))
    dictionaries = Dictionaries()
    dictionaries.register_table(
        fp.FunctionExpression, "expressions", "e", project=lambda _expression: {"operator": value},
    )
    assert dictionaries.serialize(expression) == "e0"
    assert dictionaries.table(fp.FunctionExpression) == [{"operator": expected}]


def test_projection_retains_native_values_from_exhausted_generator() -> None:
    def objects() -> Iterator[fp.Object | None]:
        repository = fp.RepositoryFactory().create_repository()
        yield repository.get_or_create(fp.ObjectData("generated"))
        yield None  # Move the iterator past the native object before checking its owner.
        gc.collect()
        assert sys.getrefcount(repository) > 2  # Generator local + getrefcount argument + retained object.

    parser = fp.Parser(DOMAIN, None, ParserOptions())
    expression = parser.get_domain().get_repository().create(fp.FunctionExpressionData(3.5))
    dictionaries = Dictionaries()
    dictionaries.register_table(
        fp.FunctionExpression, "expressions", "e", project=lambda _expression: {"objects": objects()},
    )
    dictionaries.register_table(fp.Object, "objects", "o")
    assert dictionaries.serialize(expression) == "e0"
    gc.collect()
    assert dictionaries.table(fp.FunctionExpression) == [{"objects": ["o0", None]}]
    assert dictionaries.table(fp.Object) == [{"name": "generated"}]
