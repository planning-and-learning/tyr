# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
import math
from collections.abc import Iterable, Sequence
from typing import Any, TypeAlias, cast

import pytest
from pyyggdrasil import formalism
from relation_utils import insert, intern, labels, tuples, values

from pytyr.datalog.lifted import database

Readable = database.ScratchRelation | formalism.RelationSchemaBuilderBorrowed | formalism.RelationSchema
# Reads the tests' relation builders and evaluator results.
RELATIONS = formalism.RelationRepository()
SCRATCH = database.ScratchRelations()


Rows: TypeAlias = Iterable[Sequence[object]]


def columns(*numbers: int) -> list[formalism.ColumnIndex]:
    return [formalism.ColumnIndex(number) for number in numbers]


def relation(
    labels: Sequence[int], rows: Iterable[Sequence[object]] = (), types: Sequence[formalism.ColumnType] = ()
) -> database.ScratchRelation:
    result = database.ScratchRelation(
        [
            formalism.ColumnLayoutData(
                formalism.ColumnIndex(label), types[index] if types else formalism.ColumnType.UINT32
            )
            for index, label in enumerate(labels)
        ]
    )
    for row in rows:
        insert(result, row)
    return result


def rows(value: formalism.RelationSchemaBuilderBorrowed | formalism.RelationSchema) -> set[tuple[object, ...]]:
    return set(tuples(value))


def borrow(
    source: database.ScratchRelation, edges: database.ScratchRelation, target: database.ScratchRelation
) -> tuple[
    formalism.RelationSchemaBuilderBorrowed,
    formalism.RelationSchemaBuilderBorrowed,
    formalism.RelationSchemaBuilderBorrowed,
]:
    return source.view(), edges.view(), target.view()


def change(
    added: database.ScratchRelation, removed: database.ScratchRelation
) -> tuple[formalism.RelationSchemaBuilderBorrowed, formalism.RelationSchemaBuilderBorrowed]:
    return added.view(), removed.view()


def distance(
    source: formalism.RelationSchemaBuilderBorrowed,
    edges: formalism.RelationSchemaBuilderBorrowed,
    target: formalism.RelationSchemaBuilderBorrowed,
    plan: database.DistancePlan,
) -> database.ScratchRelation:
    """The distances of borrowed inputs into a fresh relation of the plan's output columns."""
    result = database.ScratchRelation(plan.output_columns())
    database.distance(source, edges, target, plan, result, database.DistanceWorkspace(plan))
    return result


def distance_of_interned(
    source: formalism.RelationSchema,
    edges: formalism.RelationSchema,
    target: formalism.RelationSchema,
    plan: database.DistancePlan,
) -> database.ScratchRelation:
    """As distance(), of interned inputs."""
    result = database.ScratchRelation(plan.output_columns())
    database.distance(source, edges, target, plan, result, database.DistanceWorkspace(plan))
    return result


def plan_for(source: Readable, edges: Readable, target: Readable) -> database.DistancePlan:
    return database.DistancePlan(source.columns(), edges.columns(), target.columns(), formalism.ColumnIndex(99))


@pytest.mark.parametrize("interned_mask", range(8))
def test_distance_mixed_inputs_and_independent_output(interned_mask: int) -> None:
    types = [formalism.ColumnType.INT32, formalism.ColumnType.FLOAT64]
    a, b, c, d = (-7, 0.0), (3, 1.5), (5, 2.5), (9, 3.5)
    source = relation([10, 11], [(-7, -0.0), c], types)
    edges = relation([20, 21, 22, 23], [a + b, b + c, c + c], types * 2)
    target = relation([30, 31], [a, c, d], types)
    plan = plan_for(source, edges, target)
    assert plan.arity() == 2
    assert labels(plan.output_columns()) == columns(20, 21, 22, 23, 99)
    repository = formalism.RelationRepository()
    interned = [intern(repository, value)[0] for value in (source, edges, target)]
    # Interned inputs are accepted without copying only when all are interned; otherwise they are copied out.
    copied = [
        database.assign(database.ScratchRelation(), view) if interned_mask & (1 << index) else value
        for index, (value, view) in enumerate(zip((source, edges, target), interned, strict=True))
    ]
    borrowed = borrow(copied[0], copied[1], copied[2])

    # Two independent outputs of the same call; `other` is cleared below and must not affect `result`.
    if interned_mask == 7:
        result, other = (distance_of_interned(interned[0], interned[1], interned[2], plan) for _ in range(2))
    else:
        result, other = (distance(borrowed[0], borrowed[1], borrowed[2], plan) for _ in range(2))
    assert isinstance(result, database.ScratchRelation)
    expected = {a + a + (0,), a + c + (2,), c + c + (0,)}
    assert rows(result.view()) == expected
    assert labels(result.columns()) == columns(20, 21, 22, 23, 99)
    assert [column.type for column in result.columns()] == types * 2 + [formalism.ColumnType.UINT32]

    other.clear()
    source.clear()
    edges.clear()
    target.clear()
    del interned, copied, borrowed, repository, plan, source, edges, target
    gc.collect()
    assert rows(result.view()) == expected


def test_distance_canonical_nan_boolean_and_zero_length_paths() -> None:
    types = [formalism.ColumnType.BOOL, formalism.ColumnType.FLOAT64]
    source = relation([10, 11], [(True, float("nan")), (False, -0.0)], types)
    edges = relation([20, 21, 22, 23], [(True, -float("nan"), False, 0.0)], types * 2)
    target = relation([30, 31], [(True, -float("nan")), (False, 0.0)], types)
    result = distance(*borrow(source, edges, target), plan_for(source, edges, target))
    normalized = {
        tuple("nan" if isinstance(value, float) and math.isnan(value) else value for value in row)
        for row in tuples(result.view())
    }
    assert normalized == {
        (True, "nan", True, "nan", 0),
        (True, "nan", False, 0.0, 1),
        (False, 0.0, False, 0.0, 0),
    }


@pytest.mark.parametrize("intern_added", [False, True])
def test_incremental_distance_exact_deltas(intern_added: bool) -> None:
    source = relation([10], [(0,), (2,)])
    edges = relation([20, 21], [(0, 1), (1, 2), (2, 3), (0, 3)])
    target = relation([30], [(0,), (2,), (3,)])
    plan = plan_for(source, edges, target)
    evaluator = database.DistanceEvaluator(SCRATCH, plan)
    repository = formalism.RelationRepository()
    evaluator.initialize(
        *borrow(source, database.assign(database.ScratchRelation(), intern(repository, edges)[0]), target)
    )
    expected = {(0, 0, 0), (0, 2, 2), (0, 3, 1), (2, 2, 0), (2, 3, 1)}
    assert rows(evaluator.get_result()) == expected
    assert not evaluator.get_delta().added.get_rows()
    assert not evaluator.get_delta().removed.get_rows()
    assert evaluator.memory_usage() > 0

    unchanged: tuple[Rows, Rows] = ((), ())

    def update(
        *,
        source: tuple[Rows, Rows] = unchanged,
        edges: tuple[Rows, Rows] = unchanged,
        target: tuple[Rows, Rows] = unchanged,
    ) -> None:
        """Apply each input's (added, removed) rows."""
        changes = [
            relation(labels, rows)
            for labels, delta in (([10], source), ([20, 21], edges), ([30], target))
            for rows in delta
        ]
        for index in range(6):
            if (index % 2 == 0) == intern_added:
                changes[index] = database.assign(database.ScratchRelation(), intern(repository, changes[index])[0])
        evaluator.update(change(changes[0], changes[1]), change(changes[2], changes[3]), change(changes[4], changes[5]))

    before = expected
    update(edges=([(0, 2)], [(0, 3)]))
    expected = {(0, 0, 0), (0, 2, 1), (0, 3, 2), (2, 2, 0), (2, 3, 1)}
    assert rows(evaluator.get_result()) == expected
    assert rows(evaluator.get_delta().added) == expected - before
    assert rows(evaluator.get_delta().removed) == before - expected

    before = expected
    update(source=([(1,)], [(2,)]), target=([(1,)], [(0,)]))
    expected = {(0, 1, 1), (0, 2, 1), (0, 3, 2), (1, 1, 0), (1, 2, 1), (1, 3, 2)}
    assert rows(evaluator.get_result()) == expected
    assert rows(evaluator.get_delta().added) == expected - before
    assert rows(evaluator.get_delta().removed) == before - expected
    update()
    assert rows(evaluator.get_result()) == expected
    assert not evaluator.get_delta().added.get_rows()
    assert not evaluator.get_delta().removed.get_rows()


@pytest.mark.parametrize(
    "leaf",
    [
        "plan_columns",
        "result",
        "delta",
        "result_row",
        "added_row",
        "removed_row",
        "result_columns",
        "added_columns",
        "removed_columns",
        "rows",
    ],
)
def test_evaluator_borrowed_results_are_read_only_and_retain_owners(leaf: str) -> None:
    source = relation([10], [(0,)])
    edges = relation([20, 21], [(0, 1), (1, 2)])
    target = relation([30], [(2,)])
    plan = plan_for(source, edges, target)
    evaluator = database.DistanceEvaluator(SCRATCH, plan)
    evaluator.initialize(*borrow(source, edges, target))
    evaluator.update(
        change(relation([10]), relation([10])),
        change(relation([20, 21], [(0, 2)]), relation([20, 21])),
        change(relation([30]), relation([30])),
    )
    result = evaluator.get_result()
    delta = evaluator.get_delta()
    assert isinstance(result, formalism.RelationSchemaBuilderBorrowed)
    assert isinstance(delta, database.RelationDelta)

    def check_read_only(value: formalism.RelationSchemaBuilderBorrowed) -> None:
        assert not hasattr(value, "insert")
        assert not hasattr(value, "clear")
        assert not hasattr(value, "erase")
        assert value.arity() == 3
        assert labels(value.columns()) == columns(20, 21, 99)
        with pytest.raises(IndexError):
            value.row(len(value.get_rows()))

    for value in (result, delta.added, delta.removed):
        check_read_only(value)
    # Deliberate misuse: the type hints reject these calls, the bindings must too.
    with pytest.raises(AttributeError):
        cast("Any", delta).added = source
    with pytest.raises(TypeError):
        cast("Any", formalism.RelationSchemaBuilderBorrowed)()
    with pytest.raises(TypeError):
        database.RelationDelta()
    retained: Any  # each leaf is a different borrowed type
    match leaf:
        case "plan_columns":
            retained = plan.output_columns()
        case "result":
            retained = result
        case "delta":
            retained = delta
        case "result_row":
            retained = result.row(len(result.get_rows()) - 1)
        case "added_row":
            retained = delta.added.row(0)
        case "removed_row":
            retained = delta.removed.row(0)
        case "result_columns":
            retained = result.columns()
        case "added_columns":
            retained = delta.added.columns()
        case "removed_columns":
            retained = delta.removed.columns()
        case "rows":
            retained = result.get_rows()
        case _:
            raise AssertionError(leaf)
    del evaluator, delta, result, plan, source, edges, target
    gc.collect()
    if leaf.endswith("columns"):
        assert labels(retained) == columns(20, 21, 99)
    elif leaf.endswith("row"):
        assert values(retained) == (0, 2, 2 if leaf == "removed_row" else 1)
    elif leaf == "delta":
        assert rows(retained.added) == {(0, 2, 1)}
        assert rows(retained.removed) == {(0, 2, 2)}
    elif leaf == "rows":
        assert [values(row) for row in retained] == [(0, 2, 1)]
    else:
        assert tuples(retained) == [(0, 2, 1)]


def test_borrowed_relations_can_feed_another_distance_evaluation() -> None:
    source = relation([10], [(0,)])
    edges = relation([20, 21], [(0, 1)])
    target = relation([30], [(1,)])
    evaluator = database.DistanceEvaluator(SCRATCH, plan_for(source, edges, target))
    evaluator.initialize(*borrow(source, edges, target))
    borrowed = evaluator.get_result()
    next_edges = relation([40, 41, 42, 43, 44, 45])
    next_target = relation([50, 51, 52], [(0, 1, 1)])
    plan = database.DistancePlan(
        borrowed.columns(), next_edges.columns(), next_target.columns(), formalism.ColumnIndex(100)
    )
    edges_view, target_view = change(next_edges, next_target)
    assert rows(distance(borrowed, edges_view, target_view, plan).view()) == {(0, 1, 1, 0, 1, 1, 0)}


def test_distance_rejects_invalid_schema_and_input_types() -> None:
    source = relation([10], [(0,)])
    edges = relation([20, 21], [(0, 1)])
    target = relation([30], [(1,)])
    plan = plan_for(source, edges, target)
    with pytest.raises(ValueError, match="Distance: input arities must be k, 2k, k"):
        plan_for(source, relation([20]), target)
    with pytest.raises(ValueError, match="Distance: input arities must be k, 2k, k"):
        plan_for(source, edges, relation([30, 31]))
    with pytest.raises(ValueError, match="Distance: vertex field types must agree by position"):
        plan_for(source, relation([20, 21], types=[formalism.ColumnType.INT32] * 2), target)
    with pytest.raises(ValueError, match="Columns: duplicate column label"):
        database.DistancePlan(source.columns(), edges.columns(), target.columns(), formalism.ColumnIndex(20))
    with pytest.raises(ValueError, match="Relational operation: columns do not match the plan"):
        distance(*borrow(relation([11]), edges, target), plan)
    with pytest.raises(ValueError, match="Relational operation: columns do not match the plan"):
        distance(*borrow(source, edges, relation([30], types=[formalism.ColumnType.INT32])), plan)
    _, edges_view, target_view = borrow(source, edges, target)
    with pytest.raises(TypeError):
        distance(cast("Any", []), edges_view, target_view, plan)
    evaluator = database.DistanceEvaluator(SCRATCH, plan)
    with pytest.raises(RuntimeError):
        evaluator.update(
            change(source, relation([10])), change(edges, relation([20, 21])), change(target, relation([30]))
        )
    evaluator.initialize(*borrow(source, edges, target))
