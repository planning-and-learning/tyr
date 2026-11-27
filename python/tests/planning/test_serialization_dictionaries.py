# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
import json
import sys
import time
import weakref
from collections.abc import Iterator
from pathlib import Path
from typing import Literal, cast

import pytest
from pypddl.formalism import ParserOptions
from pyyggdrasil.execution import ExecutionContext
from pyyggdrasil.serialization import Dictionaries
from pyyggdrasil.serialization.table import JSONValue, Row
from typing_extensions import TypeIs

from pytyr import formalism
from pytyr.planning import SearchStatus, ground, lifted
from pytyr.planning.algorithms import brfs

TASKS = Path(__file__).resolve().parents[3] / "tests/support/data/planning/tasks"
DOMAIN = str(TASKS / "serialization/domain.pddl")
PROBLEM = str(TASKS / "serialization/serialization-1.pddl")

# The bindings check their arguments at runtime. The tests of those checks pass values the stubs reject, so they call
# the bindings through these signatures, which accept any arguments.


def _is_json(value: object) -> TypeIs[JSONValue]:
    if value is None or isinstance(value, (bool, int, float, str)):
        return True
    if isinstance(value, list):
        return all(_is_json(item) for item in cast("list[object]", value))
    return _is_row(value)


def _is_row(value: object) -> TypeIs[Row]:
    if not isinstance(value, dict):
        return False
    items = cast("dict[object, object]", value).items()
    return all(isinstance(key, str) and _is_json(item) for key, item in items)


def _rows(value: object) -> list[Row]:
    """The rows `table` returns, which the stubs leave untyped."""
    assert isinstance(value, list)
    rows: list[Row] = []
    for row in cast("list[object]", value):
        assert _is_row(row)
        rows.append(row)
    return rows


def test_default_fields_need_no_live_entity() -> None:
    assert list(formalism.RowActionLifted.Fields.__members__) == ["relation", "values"]
    assert list(formalism.AtomLiftedFluent.Fields.__members__) == ["parameters", "predicate", "terms"]
    assert list(formalism.AtomGroundFluent.Fields.__members__) == ["row"]
    assert formalism.Object.Fields.name.name == "name"
    assert formalism.Object.Fields.name.value == "name"
    assert formalism.FDRFactFluentRecordView.Fields.value.name == "value"
    assert formalism.FDRFactFluentRecordView.Fields.value.value == "value"


def test_field_enum_selection_validates_the_native_type_before_registration() -> None:
    parser = formalism.Parser(DOMAIN, ParserOptions())
    task = parser.parse_task(PROBLEM, ParserOptions())
    (atom,) = task.get_task().get_static_atoms()
    dictionaries = Dictionaries()
    with pytest.raises(TypeError):
        dictionaries.register_table(formalism.RowPredicateStatic, "bindings", "b", fields="relation")
    dictionaries.register_table(
        formalism.RowPredicateStatic,
        "bindings",
        "b",
        fields=[formalism.RowPredicateStatic.Fields.relation],
    )
    dictionaries.register_table(
        formalism.PredicateStatic, "predicates", "p", fields=[formalism.PredicateStatic.Fields.name]
    )
    dictionaries.register_table(formalism.AtomGroundStatic, "atoms", "a")
    assert dictionaries.serialize(atom) == "a0"
    assert dictionaries.table(formalism.AtomGroundStatic) == [{"row": "b0"}]
    assert dictionaries.table(formalism.RowPredicateStatic) == [{"relation": "p0"}]
    assert dictionaries.table(formalism.PredicateStatic) == [{"name": "ready"}]


@pytest.mark.parametrize("backend", ["ground", "lifted"])
def test_native_plan_tables_and_lifetime(backend: Literal["ground", "lifted"]) -> None:
    parser = formalism.Parser(DOMAIN, ParserOptions())
    task = lifted.Task(parser.parse_task(PROBLEM, ParserOptions()))
    execution = ExecutionContext(1)
    if backend == "ground":
        instantiated = task.instantiate_ground_task(execution, lifted.GroundTaskInstantiationOptions())
        assert instantiated.status == lifted.GroundTaskInstantiationStatus.SUCCESS
        task = instantiated.task
        repository = ground.StateRepository(task)
        evaluator = ground.AxiomEvaluator(task, execution)
        generator = ground.SuccessorGenerator(task, execution)
        result = brfs.ground.find_solution(task, repository, evaluator, generator, brfs.ground.Options())
        state_type = ground.StateBuilderView
        node_type = ground.NodeBuilderView
    else:
        repository = lifted.StateRepository(task)
        evaluator = lifted.AxiomEvaluator(task, execution)
        generator = lifted.SuccessorGenerator(task, execution)
        result = brfs.lifted.find_solution(task, repository, evaluator, generator, brfs.lifted.Options())
        state_type = lifted.StateBuilderView
        node_type = lifted.NodeBuilderView
    assert result.status == SearchStatus.SOLVED
    plan = result.plan
    assert plan is not None
    rejected = Dictionaries()
    with pytest.raises(ValueError, match=r"Unregistered serialization type: .*Plan"):
        rejected.serialize(plan)
    with pytest.raises(RuntimeError, match="Serialization failed"):
        rejected.tables()

    dictionaries = Dictionaries()
    dictionaries.register_table(state_type, "visited", "s")
    dictionaries.register_table(node_type, "nodes", "v")
    dictionaries.register_table(formalism.RowActionLifted, "actions", "a")
    dictionaries.register_table(formalism.ActionLifted, "action_schemas", "A", fields=["name"])
    dictionaries.register_table(formalism.FunctionTermGroundFluent, "functions", "n", fields=[])
    dictionaries.register_table(formalism.AtomGroundFluent, "atoms", "p", fields=[])
    dictionaries.register_table(formalism.AtomGroundDerived, "derived", "d", fields=[])
    assert dictionaries.table(state_type) == []
    start = plan.get_start_node()
    references = sys.getrefcount(start)
    assert dictionaries.serialize(start) == "v0"
    assert sys.getrefcount(start) > references
    (step,) = plan.get_labeled_succ_nodes()
    assert [atom.get_predicate().get_name() for atom in plan.get_start_node().get_state().fluent_atoms()] == [
        "start"
    ]
    assert [atom.get_predicate().get_name() for atom in step.get_node().get_state().fluent_atoms()] == ["done"]
    assert dictionaries.serialize(step.get_node()) == "v1"
    assert dictionaries.serialize(step.get_label()) == "a0"
    assert dictionaries.table(formalism.RowActionLifted) == [{"relation": "A0", "values": []}]
    assert dictionaries.table(formalism.ActionLifted) == [{"name": step.get_label().get_relation().get_name()}]
    assert list(formalism.RowActionLifted.Fields.__members__) == list(
        _rows(dictionaries.table(formalism.RowActionLifted))[0]
    )
    assert [row["state"] for row in _rows(dictionaries.table(node_type))] == ["s0", "s1"]
    assert dictionaries.serialize(start) == "v0"
    states = _rows(dictionaries.table(state_type))
    assert len(states) == 2
    assert states[0]["fluent_atoms"] == ["p0"]
    assert states[1]["fluent_atoms"] == ["p1"]
    assert states[0]["fluent_fterm_values"] == [["n0", 3.0]]
    assert states[1]["fluent_fterm_values"] == [["n0", 2.0]]
    assert states[0]["derived_atoms"] == ["d0"]
    assert states[1]["derived_atoms"] == []
    tables = dictionaries.tables()
    assert tables["visited"] == {"prefix": "s", "rows": states}
    encoded_tables = json.dumps(tables)
    assert json.loads(encoded_tables) == tables
    snapshot = dictionaries.tables()
    snapshot["visited"]["rows"][0]["annotation"] = {"selected": True}
    assert "annotation" not in dictionaries.tables()["visited"]["rows"][0]
    states.clear()
    assert len(_rows(dictionaries.table(state_type))) == 2
    del start, step, plan, result, generator, evaluator, repository, task, parser
    gc.collect()
    assert dictionaries.tables() == tables


def test_registration_errors_and_explicit_native_text() -> None:
    parser = formalism.Parser(DOMAIN, ParserOptions())
    task = lifted.Task(parser.parse_task(PROBLEM, ParserOptions()))
    term = next(iter(task.get_task().get_static_fterm_values())).get_fterm()
    dictionaries = Dictionaries()
    with pytest.raises(ValueError, match="prefix must end in a non-digit"):
        dictionaries.register_table(formalism.FunctionTermGroundStatic, "terms", "t0")
    dictionaries.register_table(formalism.FunctionTermGroundStatic, "terms", "t", fields=[])
    with pytest.raises(ValueError, match="Type already has a dictionary table"):
        dictionaries.register_table(formalism.FunctionTermGroundStatic, "more_terms", "u")
    assert dictionaries.serialize(term) == "t0"
    with pytest.raises(RuntimeError):
        dictionaries.register_table(formalism.FunctionTermGroundFluent, "fluent_terms", "f")
    with pytest.raises(ValueError, match="Type has no registered dictionary table"):
        dictionaries.table(formalism.FunctionTermGroundFluent)

    text = Dictionaries()
    text.register_table(formalism.FunctionTermGroundStatic, "terms", "t", project=lambda value: {"text": str(value)})
    assert text.serialize(term) == "t0"
    assert text.table(formalism.FunctionTermGroundStatic) == [{"text": str(term)}]


@pytest.mark.parametrize("nested", [False, True])
def test_unregistered_entities_name_the_missing_type_and_invalidate_registry(nested: bool) -> None:
    parser = formalism.Parser(DOMAIN, ParserOptions())
    task = parser.parse_task(PROBLEM, ParserOptions())
    term = next(iter(task.get_task().get_static_fterm_values())).get_fterm()
    dictionaries = Dictionaries()
    if nested:
        dictionaries.register_table(formalism.FunctionTermGroundStatic, "terms", "t")
    missing = "key<" if nested else "FunctionTerm"
    with pytest.raises(ValueError, match=f"^Unregistered serialization type: .*{missing}"):
        dictionaries.serialize(term)
    with pytest.raises(RuntimeError, match="Serialization failed"):
        dictionaries.serialize(term)
    with pytest.raises(RuntimeError, match="Serialization failed"):
        dictionaries.tables()


@pytest.mark.parametrize("fields", [[], ["static_atoms", "name"]])
def test_task_field_selection_limits_collected_descendants(fields: list[str]) -> None:
    parser = formalism.Parser(DOMAIN, ParserOptions())
    task = parser.parse_task(PROBLEM, ParserOptions())
    dictionaries = Dictionaries()
    dictionaries.register_table(formalism.TaskLifted, "tasks", "t", fields=fields)
    dictionaries.register_table(formalism.AtomGroundStatic, "static_atoms", "a")
    dictionaries.register_table(formalism.RowPredicateStatic, "bindings", "b")
    dictionaries.register_table(formalism.PredicateStatic, "predicates", "p")
    dictionaries.register_table(formalism.AtomGroundFluent, "fluent_atoms", "f")

    assert dictionaries.serialize(task.get_task()) == "t0"
    (row,) = _rows(dictionaries.table(formalism.TaskLifted))
    if fields == []:
        assert row == {}
        assert all(not snapshot["rows"] for name, snapshot in dictionaries.tables().items() if name != "tasks")
    else:
        assert row["name"] == "serialization-1"
        assert row["static_atoms"] == ["a0"]
        assert dictionaries.table(formalism.AtomGroundStatic) == [{"row": "b0"}]
        assert dictionaries.table(formalism.RowPredicateStatic) == [{"relation": "p0", "values": []}]
        assert [predicate["name"] for predicate in _rows(dictionaries.table(formalism.PredicateStatic))] == ["ready"]
        assert list(row) == ["name", "static_atoms"]
        assert dictionaries.table(formalism.AtomGroundFluent) == []


@pytest.mark.parametrize("fields", [None, ["name"]])
def test_projected_binding_collects_only_selected_output_fields(fields: list[str] | None) -> None:
    parser = formalism.Parser(str(TASKS / "bindings/domain.pddl"), ParserOptions())
    task = parser.parse_task(str(TASKS / "bindings/binding-1.pddl"), ParserOptions())
    (action,) = parser.get_domain().get_domain().get_actions()
    (item,) = task.get_task().get_objects()
    binding = task.get_repository().insert(formalism.RowActionLiftedData(action.index(), [item.index().value]))[0]
    calls: list[formalism.RowActionLifted] = []
    payload: dict[str, object] = {
        "scalars": [None, True, False, -(2**63), 2**63 - 1, 2**64 - 1, 1.25],
        "nested": [{"key\0é": "value\0雪"}, []],
    }

    # A row holds its objects' indices: the projection hands out the task's objects of them, serialized as references.
    objects = {obj.index().value: obj for obj in task.get_task().get_objects()}

    def project(value: formalism.RowActionLifted) -> dict[str, object]:
        calls.append(value)
        return {
            "name": value.get_relation().get_name(),
            "objects": [objects[v] for v in value.get_values()],
            "details\0": payload,
        }

    dictionaries = Dictionaries()
    dictionaries.register_table(formalism.RowActionLifted, "bindings", "b", fields=fields, project=project)
    dictionaries.register_table(formalism.Object, "objects", "o")
    dictionaries.register_table(formalism.ActionLifted, "actions", "a")
    assert dictionaries.serialize(binding) == "b0"
    assert dictionaries.serialize(binding) == "b0"
    assert calls == [binding]
    assert dictionaries.table(formalism.ActionLifted) == []
    if fields is None:
        expected = [{"name": "finish", "objects": ["o0"], "details\0": payload}]
        assert dictionaries.table(formalism.RowActionLifted) == expected
        assert json.dumps(dictionaries.tables()["bindings"]["rows"], ensure_ascii=False) == json.dumps(
            expected, ensure_ascii=False
        )
        assert [row["name"] for row in _rows(dictionaries.table(formalism.Object))] == ["item"]
    else:
        assert dictionaries.table(formalism.RowActionLifted) == [{"name": "finish"}]
        assert dictionaries.table(formalism.Object) == []


def test_invalid_projection_invalidates_registry() -> None:
    repository = formalism.Repository()
    expression = repository.insert(formalism.VariableData("e"))[0]
    dictionaries = Dictionaries()

    def project(_value: formalism.Variable) -> dict[str, object]:
        return {"unknown": object()}

    dictionaries.register_table(formalism.Variable, "expressions", "e", project=project)
    with pytest.raises(TypeError):
        dictionaries.serialize(expression)
    with pytest.raises(RuntimeError):
        dictionaries.tables()


@pytest.mark.parametrize("kind", ["list", "dict"])
def test_cyclic_projection_invalidates_registry_and_unwinds(kind: Literal["list", "dict"]) -> None:
    repository = formalism.Repository()
    value = repository.insert(formalism.ObjectData("projected"))[0]
    cycle: list[object] | dict[str, object] = [] if kind == "list" else {}
    if isinstance(cycle, list):
        cycle.append(cycle)
    else:
        cycle["self"] = cycle

    failed = Dictionaries()
    failed.register_table(formalism.Object, "objects", "o", project=lambda _value: {"cycle": cycle})
    with pytest.raises(RecursionError):
        failed.serialize(value)
    with pytest.raises(RuntimeError, match="Serialization failed"):
        failed.tables()

    shared = {"nested": [1, 2]}
    fresh = Dictionaries()
    fresh.register_table(formalism.Object, "objects", "o", project=lambda _value: {"first": shared, "second": shared})
    assert fresh.serialize(value) == "o0"
    assert fresh.table(formalism.Object) == [{"first": shared, "second": shared}]


@pytest.mark.parametrize(
    ("value", "expected"),
    [
        (formalism.ArithmeticOperatorKind.Add, "+"),
        (formalism.BooleanOperatorKind.Ge, ">="),
        (formalism.NumericEffectOperatorKind.Increase, "increase"),
        (formalism.OptimizationDirection.Minimize, "minimize"),
    ],
)
def test_projection_uses_native_enum_text(value: object, expected: str) -> None:
    repository = formalism.Repository()
    expression = repository.insert(formalism.VariableData("e"))[0]
    dictionaries = Dictionaries()
    dictionaries.register_table(
        formalism.Variable,
        "expressions",
        "e",
        project=lambda _expression: {"operation": value},
    )
    assert dictionaries.serialize(expression) == "e0"
    assert dictionaries.table(formalism.Variable) == [{"operation": expected}]


def test_projection_retains_native_values_from_exhausted_generator() -> None:
    def objects() -> Iterator[formalism.Object | None]:
        repository = formalism.Repository()
        yield repository.insert(formalism.ObjectData("generated"))[0]
        yield None  # Move the iterator past the native object before checking its owner.
        gc.collect()
        assert sys.getrefcount(repository) > 2  # Generator local + getrefcount argument + retained object.

    repository = formalism.Repository()
    expression = repository.insert(formalism.VariableData("e"))[0]
    dictionaries = Dictionaries()
    dictionaries.register_table(
        formalism.Variable,
        "expressions",
        "e",
        project=lambda _expression: {"objects": objects()},
    )
    dictionaries.register_table(formalism.Object, "objects", "o")
    assert dictionaries.serialize(expression) == "e0"
    gc.collect()
    assert dictionaries.table(formalism.Variable) == [{"objects": ["o0", None]}]
    assert dictionaries.table(formalism.Object) == [{"name": "generated"}]


class _Holder:
    def __init__(self, dictionaries: Dictionaries) -> None:
        self.dictionaries = dictionaries


def test_serialize_time_per_call_does_not_grow_with_the_tables() -> None:
    # serialize keeps every value alive for the dictionaries. With nb::keep_alive, each call scanned all earlier values,
    # so a call cost time linear in the calls before it; the second half of the atoms then took several times as long
    # as the first. Atoms with a projection that serializes their objects, as table writers do, make 3 calls each.
    count = 160
    names = [f"o{index}" for index in range(count)]
    edges = " ".join(f"(edge {a} {b})" for a in names for b in names if a != b)
    domain = "(define (domain grid) (:requirements :strips) (:predicates (edge ?a ?b) (at ?a)))"
    problem = (
        f"(define (problem grid-1) (:domain grid) (:objects {' '.join(names)}) (:init {edges} (at o0)) (:goal (at o1)))"
    )
    parser = formalism.Parser(domain, None, ParserOptions())
    task = parser.parse_task(problem, None, ParserOptions())
    atoms = list(task.get_task().get_static_atoms())
    assert len(atoms) >= count * (count - 1)
    holder = _Holder(Dictionaries())
    dictionaries = holder.dictionaries
    # Through a proxy: a projection that held the dictionaries would close a cycle through native code that gc cannot
    # see (Dictionaries itself takes no weak references).
    this = weakref.proxy(holder)

    def project(atom: formalism.AtomGroundStatic) -> dict[str, object]:
        return {"objects": [this.dictionaries.serialize(item) for item in atom.get_row().resolve_objects()]}

    dictionaries.register_table(formalism.AtomGroundStatic, "atoms", "a", project=project)
    dictionaries.register_table(formalism.Object, "objects", "o")
    half = len(atoms) // 2
    start = time.perf_counter()
    for atom in atoms[:half]:
        dictionaries.serialize(atom)
    first = time.perf_counter() - start
    start = time.perf_counter()
    for atom in atoms[half:]:
        dictionaries.serialize(atom)
    second = time.perf_counter() - start
    assert len(_rows(dictionaries.table(formalism.AtomGroundStatic))) == len(atoms)
    assert second < 2.5 * first + 0.05, (first, second)
