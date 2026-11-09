# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
from collections.abc import Iterable, Sequence
from typing import Any, cast

import pytest

from pyyggdrasil import formalism
from pytyr import database as db

QueryRecord = (
    db.QueryInputData
    | db.QueryEmptyData
    | db.QueryJoinData
    | db.QueryProjectData
    | db.QueryRenameData
    | db.QuerySelectEqualData
    | db.QuerySelectValueData
    | db.QueryUnionData
    | db.QueryDifferenceData
    | db.QueryDistanceData
    | db.QueryGenericJoinData
    | db.QuerySemijoinData
    | db.QueryAntijoinData
)
Readable = formalism.Relation | formalism.BorrowedRelation | formalism.RelationView
# Reads the tests' relation builders and evaluator results.
RELATIONS = formalism.RelationRepository()


def columns(*labels: int) -> list[formalism.ColumnIndex]:
    return [formalism.ColumnIndex(label) for label in labels]


def relation(
    labels: Sequence[int], values: Iterable[Sequence[object]] = (), types: Sequence[formalism.ColumnType] = ()
) -> formalism.Relation:
    result = formalism.Relation(
        [
            formalism.ColumnLayout(formalism.ColumnIndex(label), types[index] if types else formalism.ColumnType.UINT32)
            for index, label in enumerate(labels)
        ]
    )
    for value in values:
        result.insert(value)
    return result


def query(repo: db.QueryRepository, data: QueryRecord) -> db.Query:
    """Interns the concrete record and returns its type-erased query."""
    concrete, _ = repo.insert(data)
    return repo.insert(db.QueryData(concrete))[0]


def inputs_of(
    repo: db.QueryRepository, relations: Sequence[Readable], slots: Iterable[int] | None = None
) -> list[db.Query]:
    slots = range(len(relations)) if slots is None else slots
    return [query(repo, db.QueryInputData(slot, value.columns())) for slot, value in zip(slots, relations, strict=True)]


def rows(value: Iterable[formalism.RelationRow]) -> set[tuple[object, ...]]:
    return {tuple(row) for row in value}


def borrow(relations: Iterable[formalism.Relation]) -> list[formalism.BorrowedRelation]:
    return [RELATIONS.view(value) for value in relations]


def change(added: formalism.Relation, removed: formalism.Relation) -> tuple[formalism.BorrowedRelation, formalism.BorrowedRelation]:
    return RELATIONS.view(added), RELATIONS.view(removed)


def changes(
    schemas: Sequence[Sequence[int]],
    additions: Sequence[Iterable[Sequence[object]]],
    removals: Sequence[Iterable[Sequence[object]]],
) -> list[tuple[formalism.BorrowedRelation, formalism.BorrowedRelation]]:
    return [change(relation(s, a), relation(s, r)) for s, a, r in zip(schemas, additions, removals, strict=True)]


@pytest.mark.parametrize("measured", [True, False])
def test_optimizer_preserves_typed_shared_roots_and_output_order(measured: bool) -> None:
    types = [formalism.ColumnType.INT32, formalism.ColumnType.FLOAT64]
    left = relation([0, 1], [(1, 2.5), (2, 3.5), (3, 2.5)], types)
    right = relation([1, 2], [(2.5, 7), (3.5, 8)], types[::-1])
    repo = db.QueryRepository()
    a, b = inputs_of(repo, [left, right])
    shared = query(repo, db.QueryJoinData(a, b))
    projected = query(repo, db.QueryProjectData(shared, columns(2, 0)))
    selected = query(repo, db.QuerySelectValueData(shared, formalism.ColumnIndex(0), formalism.ColumnType.INT32, 1))
    roots = [projected, selected, query(repo, db.QueryRenameData(projected, columns(8, 9)))]
    assert query(repo, db.QueryJoinData(a, b)) == shared
    assert hash(query(repo, db.QueryJoinData(a, b))) == hash(shared)
    original = db.compile(roots)
    optimized = db.optimize(roots, db.collect_statistics(borrow([left, right])) if measured else db.Statistics())
    baseline, actual = db.QueryEvaluator(RELATIONS, original), db.QueryEvaluator(RELATIONS, optimized)
    baseline.evaluate(borrow([left, right]))
    actual.evaluate(borrow([left, right]))
    assert optimized.root_count == 3
    assert optimized.node_count > 0
    assert optimized.explain()
    for index in range(3):
        assert tuple(actual.get_result(index).columns()) == tuple(baseline.get_result(index).columns())
        assert rows(actual.get_result(index)) == rows(baseline.get_result(index))
    assert rows(actual.get_result()) == {(7, 1), (8, 2), (7, 3)}
    assert actual.memory_usage() > 0


def test_records_views_and_insertion_identity() -> None:
    repo = db.QueryRepository()
    a, b = inputs_of(repo, [relation([0, 1]), relation([1, 2])])
    data = db.QueryJoinData(a, b)
    assert data == db.QueryJoinData(a, b)
    assert data.lhs == a.index()
    assert data.rhs == b.index()
    join, created = repo.insert(data)
    assert created
    assert isinstance(join, db.QueryJoin)
    assert isinstance(join.index(), db.QueryJoinIndex)
    assert join.get_lhs() == a
    assert join.get_rhs() == b
    assert tuple(join.columns()) == tuple(columns(0, 1, 2))
    assert repo.insert(db.QueryJoinData(a, b)) == (join, False)
    erased, _ = repo.insert(db.QueryData(join))
    variant = erased.get_variant()
    assert isinstance(variant, db.QueryJoin)
    assert variant == join
    selected, _ = repo.insert(db.QuerySelectEqualData(erased, formalism.ColumnIndex(0), formalism.ColumnIndex(2)))
    assert selected.get_arg() == erased
    assert selected.get_lhs_column() == formalism.ColumnIndex(0)
    distance, _ = repo.insert(
        db.QueryDistanceData(a, inputs_of(repo, [relation([5, 6, 7, 8])])[0], a, formalism.ColumnIndex(9))
    )
    assert distance.get_distance_column() == formalism.ColumnIndex(9)
    generic, _ = repo.insert(db.QueryGenericJoinData([a, b], columns(1, 0, 2)))
    assert tuple(generic.columns()) == tuple(columns(0, 1, 2))
    empty, _ = repo.insert(db.QueryEmptyData(a.columns()))
    assert tuple(empty.columns()) == tuple(columns(0, 1))
    assert isinstance(repo.insert(db.QueryUnionData(a, a))[0], db.QueryUnion)
    assert isinstance(repo.insert(db.QueryDifferenceData(a, a))[0], db.QueryDifference)


def test_semijoin_and_antijoin_filter_their_lhs() -> None:
    edges = relation([0, 1], [(1, 7), (2, 8), (3, 9)])
    reached = relation([1], [(7,), (9,)])
    repo = db.QueryRepository()
    edges_query, reached_query = inputs_of(repo, [edges, reached])
    semijoin, _ = repo.insert(db.QuerySemijoinData(edges_query, reached_query))
    assert isinstance(semijoin, db.QuerySemijoin)
    assert isinstance(semijoin.index(), db.QuerySemijoinIndex)
    assert tuple(semijoin.columns()) == tuple(columns(0, 1))
    antijoin, _ = repo.insert(db.QueryAntijoinData(edges_query, reached_query))
    assert isinstance(antijoin, db.QueryAntijoin)
    assert antijoin.get_rhs() == reached_query
    roots = [repo.insert(db.QueryData(semijoin))[0], repo.insert(db.QueryData(antijoin))[0]]
    evaluation = db.QueryEvaluator(RELATIONS, db.compile(roots))
    evaluation.evaluate(borrow([edges, reached]))
    assert rows(evaluation.get_result(0)) == {(1, 7), (3, 9)}
    assert rows(evaluation.get_result(1)) == {(2, 8)}


def test_difference_projection_barrier_and_nullary_support() -> None:
    source = relation([0, 1], [(1, 7), (1, 8)])
    removed = relation([0, 1], [(1, 7)])
    repo = db.QueryRepository()
    source_query, removed_query = inputs_of(repo, [source, removed])
    surviving = query(
        repo, db.QueryProjectData(query(repo, db.QueryDifferenceData(source_query, removed_query)), columns(0))
    )
    roots = [surviving, query(repo, db.QueryProjectData(surviving, columns()))]
    evaluation = db.QueryEvaluator(RELATIONS, db.optimize(roots, db.collect_statistics(borrow([source, removed]))))
    evaluation.evaluate(borrow([source, removed]))
    assert rows(evaluation.get_result()) == {(1,)}
    assert rows(evaluation.get_result(1)) == {()}


def test_incremental_batch_changes_projection_union_and_difference() -> None:
    schemas = [[0, 1], [1, 2], [0, 2]]
    current = [{(1, 7), (1, 8), (2, 8)}, {(7, 9), (8, 9)}, {(2, 9)}]
    inputs = [relation(schema, values) for schema, values in zip(schemas, current, strict=True)]
    repo = db.QueryRepository()
    a, b, blocked = inputs_of(repo, inputs)
    projected = query(repo, db.QueryProjectData(query(repo, db.QueryJoinData(a, b)), columns(0, 2)))
    visible = query(repo, db.QueryDifferenceData(projected, blocked))
    combined = query(repo, db.QueryUnionData(visible, query(repo, db.QueryEmptyData(inputs[2].columns()))))
    roots = [combined, query(repo, db.QueryProjectData(visible, columns()))]
    full, maintained = (
        db.QueryEvaluator(RELATIONS, db.compile(roots)),
        db.IncrementalQueryEvaluator(RELATIONS, db.optimize(roots, db.collect_statistics(borrow(inputs)))),
    )
    full.evaluate(borrow(inputs))
    maintained.initialize(borrow(inputs))
    for index in range(2):
        assert rows(maintained.get_result(index)) == rows(full.get_result(index))
        assert maintained.get_delta(index).added.empty()
        assert maintained.get_delta(index).removed.empty()
    assert maintained.memory_usage() > 0
    steps: list[tuple[list[set[tuple[int, int]]], list[set[tuple[int, int]]]]] = [
        ([set(), set(), set()], [{(1, 7)}, set(), set()]),
        ([{(3, 7)}, {(7, 10)}, set()], [{(1, 8)}, {(7, 9)}, {(2, 9)}]),
        ([set(), set(), {(3, 10), (2, 9)}], [set(), set(), set()]),
        ([set(), set(), set()], [set(), set(), set()]),
    ]
    for additions, removals in steps:
        before = [rows(maintained.get_result(i)) for i in range(2)]
        for i in range(3):
            current[i] = (current[i] - removals[i]) | additions[i]
        maintained.update(changes(schemas, additions, removals))
        full.evaluate(borrow(relation(s, v) for s, v in zip(schemas, current, strict=True)))
        for index in range(2):
            after = rows(full.get_result(index))
            assert rows(maintained.get_result(index)) == after
            assert rows(maintained.get_delta(index).added) == after - before[index]
            assert rows(maintained.get_delta(index).removed) == before[index] - after


def test_triangle_and_self_join() -> None:
    inputs = [
        relation([0, 1], [(1, 2), (2, 3), (3, 1)]),
        relation([1, 2], [(2, 3), (3, 1), (1, 2)]),
        relation([2, 0], [(3, 1), (1, 2), (2, 3)]),
    ]
    repo = db.QueryRepository()
    a, b, c = inputs_of(repo, inputs)
    triangle = query(repo, db.QueryJoinData(query(repo, db.QueryJoinData(a, b)), c))
    roots = [triangle, query(repo, db.QueryJoinData(a, a))]
    evaluator = db.QueryEvaluator(RELATIONS, db.optimize(roots, db.collect_statistics(borrow(inputs))))
    evaluator.evaluate(borrow(inputs))
    assert rows(evaluator.get_result()) == {(1, 2, 3), (2, 3, 1), (3, 1, 2)}
    assert rows(evaluator.get_result(1)) == rows(RELATIONS.view(inputs[0]))


def test_borrowed_and_interned_inputs_statistics_and_owners() -> None:
    original = relation([0], [(1,), (2,)])
    store = formalism.RelationRepository()
    interned = store.insert(original)[0]
    repo = db.QueryRepository()
    root = inputs_of(repo, [original])[0]
    evaluator = db.QueryEvaluator(RELATIONS, db.compile([root]))
    evaluator.evaluate([interned])
    borrowed = evaluator.get_result()
    independent = db.assign(formalism.Relation(), borrowed)
    statistics = db.collect_statistics([borrowed])
    assert statistics.inputs[0].rows == 2
    assert statistics.inputs[0].distinct == {formalism.ColumnIndex(0): 2}
    assert db.collect_statistics([interned]).inputs[0].rows == 2
    second = db.QueryEvaluator(RELATIONS, db.compile([root]))
    second.evaluate([borrowed])
    row = second.get_result()[0]
    assert not hasattr(second.get_result(), "clear")
    del root, repo, evaluator, second, borrowed, original, store, interned
    gc.collect()
    assert tuple(row) in {(1,), (2,)}
    assert rows(RELATIONS.view(independent)) == {(1,), (2,)}


def test_query_handle_retains_repository_and_compiled_plan_is_independent() -> None:
    source = relation([4], [(9,)])
    repo = db.QueryRepository()
    handle = query(repo, db.QueryInputData(0, relation([4]).columns()))
    other = query(db.QueryRepository(), db.QueryInputData(0, relation([4]).columns()))
    assert handle != other
    assert len({handle, other}) == 2
    plan = db.compile([handle])
    del repo
    gc.collect()
    assert tuple(handle.columns()) == tuple(columns(4))
    del handle
    gc.collect()
    evaluator = db.QueryEvaluator(RELATIONS, plan)
    del plan
    evaluator.evaluate(borrow([source]))
    assert rows(evaluator.get_result()) == {(9,)}


@pytest.mark.parametrize("incremental", [False, True])
def test_distance_boundary_and_sparse_input_slots(incremental: bool) -> None:
    source = relation([10], [(0,)])
    edges = relation([20, 21], [(0, 1), (1, 2)])
    target = relation([30], [(2,)])
    repo = db.QueryRepository()
    s, e, t = inputs_of(repo, [source, edges, target], slots=[1, 3, 4])
    distance = query(repo, db.QueryDistanceData(s, e, t, formalism.ColumnIndex(99)))
    optimized = db.optimize([distance])
    unused = formalism.Relation()
    if not incremental:
        evaluator = db.QueryEvaluator(RELATIONS, optimized)
        evaluator.evaluate(borrow([unused, source, unused, edges, target]))
        assert rows(evaluator.get_result()) == {(0, 2, 2)}
    else:
        maintained = db.IncrementalQueryEvaluator(RELATIONS, optimized)
        maintained.initialize(borrow([unused, source, unused, edges, target]))
        assert rows(maintained.get_result()) == {(0, 2, 2)}
        no_change = change(formalism.Relation(), formalism.Relation())
        maintained.update(
            [
                no_change,
                change(relation([10]), relation([10])),
                no_change,
                change(relation([20, 21], [(0, 2)]), relation([20, 21])),
                change(relation([30]), relation([30])),
            ]
        )
        assert rows(maintained.get_result()) == {(0, 2, 1)}
        assert rows(maintained.get_delta().added) == {(0, 2, 1)}
        assert rows(maintained.get_delta().removed) == {(0, 2, 2)}


def test_statistics_and_input_validation() -> None:
    value = relation([0], [(1,)])
    repo = db.QueryRepository()
    source = inputs_of(repo, [value])[0]
    assert db.optimize([source]).root_count == 1
    known = db.Statistics(objects=4, inputs={0: db.RelationStatistics(1, {formalism.ColumnIndex(0): 1})})
    assert db.optimize([source], known).root_count == 1
    with pytest.raises(ValueError, match="Optimizer: invalid row count"):
        db.optimize([source], db.Statistics(inputs={0: db.RelationStatistics(-1, {formalism.ColumnIndex(0): 1})}))
    evaluator = db.QueryEvaluator(RELATIONS, db.compile([source]))
    with pytest.raises(TypeError):  # deliberate misuse, rejected by the type hints too
        evaluator.evaluate(cast("Any", [object()]))
    with pytest.raises(IndexError):
        repo.insert(db.QueryProjectData(source, columns(5)))
    incremental = db.IncrementalQueryEvaluator(RELATIONS, db.compile([source]))
    incremental.initialize(borrow([value]))
    with pytest.raises(ValueError, match="Incremental query: missing input delta"):
        incremental.update([])


@pytest.mark.parametrize("measured", [True, False])
def test_planning_is_deterministic(measured: bool) -> None:
    inputs = [relation([i, i + 1], [(1, 1), (2, 2)]) for i in range(4)]
    repo = db.QueryRepository()
    queries = inputs_of(repo, inputs)
    root = queries[0]
    for other in queries[1:]:
        root = query(repo, db.QueryJoinData(root, other))
    known = db.collect_statistics(borrow(inputs)) if measured else db.Statistics()
    first = db.optimize([root], known)
    second = db.optimize([root], known)
    assert first.explain() == second.explain()
    evaluator = db.QueryEvaluator(RELATIONS, first)
    evaluator.evaluate(borrow(inputs))
    assert rows(evaluator.get_result()) == {(1, 1, 1, 1, 1), (2, 2, 2, 2, 2)}


@pytest.mark.parametrize(
    ("column_type", "constant"), [(formalism.ColumnType.INT32, -3), (formalism.ColumnType.FLOAT64, -3.0), (formalism.ColumnType.BOOL, True)]
)
def test_selection_constants_follow_column_types_and_equality(column_type: formalism.ColumnType, constant: object) -> None:
    other = {formalism.ColumnType.INT32: 7, formalism.ColumnType.FLOAT64: 7.0, formalism.ColumnType.BOOL: False}[column_type]
    source = relation([0, 1], [(constant, constant), (constant, other), (other, other)], [column_type] * 2)
    repo = db.QueryRepository()
    source_query = inputs_of(repo, [source])[0]
    equal = query(repo, db.QuerySelectEqualData(source_query, formalism.ColumnIndex(0), formalism.ColumnIndex(1)))
    selected = query(repo, db.QuerySelectValueData(equal, formalism.ColumnIndex(0), column_type, constant))
    evaluator = db.QueryEvaluator(RELATIONS, db.optimize([selected]))
    evaluator.evaluate(borrow([source]))
    assert rows(evaluator.get_result()) == {(constant, constant)}
    with pytest.raises(TypeError):
        db.QuerySelectValueData(source_query, formalism.ColumnIndex(0), column_type, "invalid")
    with pytest.raises(ValueError, match="Query: constant type does not match the selected column"):
        repo.insert(db.QuerySelectValueData(source_query, formalism.ColumnIndex(0), formalism.ColumnType.UINT64, 1))
