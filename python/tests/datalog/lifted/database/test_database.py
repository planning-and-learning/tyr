# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
import math
from collections.abc import Iterable
from typing import Any, cast

import pytest
from pyyggdrasil import formalism
from relation_utils import insert, intern, labels, rename, tuples, values

import pytyr
from pytyr.datalog.lifted import database


def columns(*labels: int) -> list[formalism.ColumnIndex]:
    return [formalism.ColumnIndex(label) for label in labels]


def typed(
    labels: Iterable[formalism.ColumnIndex], types: Iterable[formalism.ColumnType]
) -> list[formalism.ColumnLayout]:
    return [formalism.ColumnLayout(label, column_type) for label, column_type in zip(labels, types, strict=True)]


def layouts(*labels: int) -> list[formalism.ColumnLayout]:
    return typed(columns(*labels), [formalism.ColumnType.UINT32] * len(labels))


def test_relation_rows_and_validation() -> None:
    assert database is pytyr.datalog.lifted.database
    exported: list[str] = vars(database)["__all__"]  # the generated type hints do not list __all__
    assert all(hasattr(database, name) for name in exported)
    relation = database.ScratchRelation(layouts(10, 20))
    schema = relation.columns()
    assert isinstance(schema, list)
    assert len(schema) == 2
    assert schema == layouts(10, 20)
    assert all(type(column) is formalism.ColumnLayout for column in schema)
    assert labels(schema) == columns(10, 20)
    assert relation.arity() == 2
    assert relation.empty()
    assert insert(relation, [3, 4]) == insert(relation, [3, 4])
    assert not hasattr(relation, "rows")  # a builder's rows are read through a repository
    repository = formalism.RelationRepository()
    view = relation.view()
    assert isinstance(view, formalism.RelationSchemaBuilder)
    row = view.row(0)
    assert isinstance(row, formalism.RowSchemaBorrowed)  # a scratch relation's row is a record, not interned
    assert relation.size() == 1
    assert not relation.empty()
    assert values(row) == (3, 4)
    assert tuples(view) == [(3, 4)]
    with pytest.raises(IndexError):
        view.row(1)
    assert row.get(1) == 4
    assert row.get(0) == 3
    with pytest.raises(IndexError):
        row.get(2)
    with pytest.raises(TypeError):  # read-only: no __setitem__ in the type hints either
        cast("Any", row)[0] = 7
    with pytest.raises(ValueError, match="a row has one value per column"):
        relation.insert([formalism.encode(1, formalism.ColumnType.UINT32)])
    with pytest.raises(ValueError, match="Columns: duplicate column label"):
        database.ScratchRelation(layouts(1, 1))

    for value in range(5000):
        insert(relation, [value, 0])
    row = view.row(insert(relation, [3, 4]))  # rows are in tuple order: the inserts moved (3, 4); its position now
    del relation, view, repository
    gc.collect()
    assert values(row) == (3, 4)


def test_nullary_relation() -> None:
    relation = database.ScratchRelation()
    assert relation.arity() == 0
    assert relation.size() == 0
    assert isinstance(relation.columns(), list)
    assert relation.columns() == []
    repository = formalism.RelationRepository()
    view = relation.view()
    assert view.get_rows() == []
    relation.insert([])
    relation.insert([])
    assert relation.size() == 1
    row = view.row(0)
    assert values(row) == ()
    assert tuples(view) == [()]
    with pytest.raises(IndexError):
        row.get(0)


def test_pooled_rows_keep_their_owners_alive() -> None:
    repository = formalism.RelationRepository()
    relation = database.ScratchRelation(layouts(0, 1))
    assert isinstance(relation, database.ScratchRelation)
    insert(relation, [3, 4])
    row = relation.view().row(0)
    del relation
    gc.collect()

    other = database.ScratchRelation(layouts(10, 20))
    assert other.empty()
    insert(other, [5, 6])
    assert values(row) == (3, 4)
    del other
    gc.collect()
    with pytest.raises(ValueError, match="Columns: duplicate column label"):
        database.ScratchRelation(layouts(10, 10))
    reused = database.ScratchRelation(layouts(30, 40))
    assert reused.empty()
    insert(reused, [7, 8])
    assert values(row) == (3, 4)
    del reused, repository
    gc.collect()
    assert values(row) == (3, 4)


def test_row_lists_keep_pooled_rows_alive() -> None:
    repository = formalism.RelationRepository()
    relation = database.ScratchRelation(layouts(0, 1))
    insert(relation, [3, 4])
    insert(relation, [5, 6])
    rows = relation.view().get_rows()
    assert isinstance(rows, list)
    del relation, repository
    gc.collect()

    row = rows[0]
    assert [values(other) for other in rows] == [(3, 4), (5, 6)]
    del rows
    gc.collect()
    assert values(row) == (3, 4)


def test_interned_relations_use_typed_identity_and_independent_storage() -> None:
    repository = formalism.RelationRepository()
    builder = database.ScratchRelation(layouts(10, 20))
    insert(builder, [3, 4])
    insert(builder, [5, 6])
    result = intern(repository, builder)
    assert isinstance(result, tuple)
    relation, created = result
    duplicate, duplicate_created = intern(repository, builder)
    assert created is True
    assert duplicate_created is False
    del result
    assert isinstance(relation, formalism.RelationSchema)
    assert isinstance(relation.index(), formalism.RelationSchemaIndex)
    assert relation == duplicate
    assert hash(relation) == hash(duplicate)
    assert len({relation, duplicate}) == 1
    assert len(repository) == 1
    assert not hasattr(relation, "insert")
    assert not hasattr(relation, "clear")

    other_repository = formalism.RelationRepository()
    assert intern(other_repository, builder)[0] != relation
    assert rename(repository, relation, columns(10, 20)) == relation

    builder.clear()
    insert(builder, [8, 9])
    assert tuples(relation) == [(3, 4), (5, 6)]
    assert tuples(builder.view()) == [(8, 9)]
    assert relation.arity() == 2
    assert len(relation.get_rows()) == 2
    assert relation.get_rows()
    assert values(relation.row(1)) == (5, 6)
    with pytest.raises(IndexError):
        relation.row(2)

    renamed = rename(repository, relation, columns(30, 40))
    assert labels(renamed.columns()) == columns(30, 40)
    assert tuples(renamed) == [(3, 4), (5, 6)]
    assert renamed != relation
    assert rename(repository, renamed, columns(10, 20)) == relation
    with pytest.raises(ValueError, match="Columns: renaming must preserve arity"):
        rename(repository, relation, columns(30))
    with pytest.raises(ValueError, match="Repository: a relation's column labels are distinct"):
        rename(repository, relation, columns(30, 30))

    # Every borrowed layer must keep the repository alive, without keeping the
    # mutable builder alive or depending on its buffers.
    row = relation.row(0)
    rows = renamed.get_rows()
    del relation, duplicate, renamed, repository, other_repository, builder
    gc.collect()
    assert values(row) == (3, 4)
    assert [values(other) for other in rows] == [(3, 4), (5, 6)]


def test_interned_nullary_relations_and_repository_reset() -> None:
    repository = formalism.RelationRepository()
    builder = database.ScratchRelation()
    false = intern(repository, builder)[0]
    builder.insert([])
    true = intern(repository, builder)[0]
    assert false.get_rows() == []
    assert tuples(true) == [()]
    assert false != true
    assert len(repository) == 2
    del false, true
    repository.clear()
    assert len(repository) == 0
    assert tuples(intern(repository, builder)[0]) == [()]


def test_interning_ignores_row_insertion_order() -> None:
    repository = formalism.RelationRepository()
    seed = database.ScratchRelation(layouts(10, 20))
    insert(seed, [9, 8])
    intern(repository, seed)[0]

    builder = database.ScratchRelation(layouts(10, 20))
    insert(builder, [3, 4])
    insert(builder, [9, 8])
    forward = intern(repository, builder)[0]
    builder.clear()
    insert(builder, [9, 8])
    insert(builder, [3, 4])
    insert(builder, [3, 4])
    reverse = intern(repository, builder)[0]

    assert forward == reverse
    assert forward.index() == reverse.index()
    assert hash(forward) == hash(reverse)
    assert len(repository) == 2
    # Row IDs follow the repository's row-interning history; canonical
    # relations enumerate the sorted IDs, independent of builder order.
    assert tuples(forward) == [(3, 4), (9, 8)]  # canonical: tuple order
    assert tuples(reverse) == [(3, 4), (9, 8)]  # canonical: tuple order


def test_copy_and_assign_remap_relations_and_retain_unpacked_owner() -> None:
    source_repository = formalism.RelationRepository()
    target_repository = formalism.RelationRepository()
    builder = database.ScratchRelation(layouts(7, 3))
    insert(builder, [4, 5])
    source, created = intern(source_repository, builder)
    assert created is True
    target_result = intern(target_repository, source)
    target, copied = target_result
    assert copied is True
    assert intern(target_repository, source) == (target, False)
    assert intern(target_repository, target) == (target, False)
    assert labels(target.columns()) == columns(7, 3)
    output = database.ScratchRelation(layouts(3, 7))
    insert(output, [99, 88])
    assert database.assign(output, target) is output
    assert labels(output.columns()) == columns(7, 3)
    assert tuples(output.view()) == [(4, 5)]
    assert database.assign(output, output.view()) is output
    assert labels(output.columns()) == columns(7, 3)
    assert tuples(output.view()) == [(4, 5)]
    unary = database.ScratchRelation(layouts(99))
    insert(unary, [77])
    assert database.assign(unary, output.view()) is unary
    assert labels(unary.columns()) == columns(7, 3)
    assert tuples(unary.view()) == [(4, 5)]
    del target_result, source, source_repository, target_repository
    del builder, output, unary
    gc.collect()
    assert tuples(target) == [(4, 5)]
    assert not hasattr(database, "intern_relation")


def test_mixed_column_types_and_float_canonicalization() -> None:
    types = [formalism.ColumnType.BOOL, formalism.ColumnType.FLOAT64, formalism.ColumnType.INT32]
    relation = database.ScratchRelation(typed(columns(10, 20, 30), types))
    assert [column.type for column in relation.columns()] == types
    first = insert(relation, [True, -0.0, -7])
    assert insert(relation, [True, 0.0, -7]) == first
    nan = insert(relation, [False, float("nan"), 9])
    assert insert(relation, [False, -float("nan"), 9]) == nan
    repository = formalism.RelationRepository()
    # Rows are in tuple order: (False, nan, 9) went before (True, 0.0, -7), so their positions are read now.
    row = relation.view().row(insert(relation, [True, 0.0, -7]))
    assert values(row) == (True, 0.0, -7)
    assert [type(value) for value in values(row)] == [bool, float, int]
    value = relation.view().row(insert(relation, [False, float("nan"), 9])).get(1)
    assert isinstance(value, float)
    assert math.isnan(value)
    interned, _ = intern(repository, relation)
    renamed = rename(repository, interned, columns(1, 2, 3))
    assert [column.type for column in renamed.columns()] == types
    output = database.ScratchRelation()
    database.assign(output, renamed)
    assert values(output.view().row(1)) == (True, 0.0, -7)  # tuple order: (False, nan, 9) first
    with pytest.raises(ValueError, match="Columns: duplicate column label"):
        database.ScratchRelation(typed(columns(1, 1), [formalism.ColumnType.BOOL] * 2))
    del relation, interned, renamed, repository
    gc.collect()
    assert values(row) == (True, 0.0, -7)


def test_operations_take_plans_outputs_and_workspaces() -> None:
    repository = formalism.RelationRepository()
    left_builder = database.ScratchRelation(layouts(0, 1))
    right_builder = database.ScratchRelation(layouts(1, 2))
    for row in ([1, 2], [3, 4]):
        insert(left_builder, row)
    insert(right_builder, [2, 5])
    left_operand, right_operand = left_builder.view(), intern(repository, right_builder)[0]
    workspace = database.Workspace()

    join = database.JoinPlan(left_operand.columns(), right_operand.columns())
    joined = database.ScratchRelation(join.output_columns())
    database.join(left_operand, right_operand, join, joined, workspace)
    assert tuples(joined.view()) == [(1, 2, 5)]
    semi = database.ScratchRelation(join.left_columns())
    database.semijoin(left_operand, right_operand, join, semi, workspace)
    assert tuples(semi.view()) == [(1, 2)]
    database.antijoin(left_operand, right_operand, join, semi, workspace)
    assert tuples(semi.view()) == [(3, 4)]

    projection = database.ProjectionPlan(left_operand.columns(), columns(1))
    projected = database.ScratchRelation(projection.output_columns())
    database.project(left_operand, projection, projected, workspace)
    assert sorted(tuples(projected.view())) == [(2,), (4,)]

    other = database.ScratchRelation(layouts(0, 1))
    insert(other, [3, 4])
    insert(other, [7, 7])
    sets = database.SetPlan(left_operand.columns(), other.columns())
    union = database.ScratchRelation(sets.output_columns())
    database.union(left_operand, other.view(), sets, union)
    assert union.size() == 3
    database.difference(left_operand, other.view(), sets, union)
    assert tuples(union.view()) == [(1, 2)]

    equal = database.ScratchRelation(other.columns())
    database.select_equal_columns(
        other.view(), database.SelectEqualPlan(other.columns(), *columns(0, 1)), equal
    )
    assert tuples(equal.view()) == [(7, 7)]
    value = database.SelectValuePlan(other.columns(), formalism.ColumnIndex(0), formalism.ColumnType.UINT32, 3)
    database.select_equal_value(other.view(), value, equal)
    assert tuples(equal.view()) == [(3, 4)]

    with pytest.raises(ValueError, match="Relational operation: input schemas must have the same column order"):
        database.SetPlan(left_operand.columns(), right_operand.columns())
    with pytest.raises(ValueError, match="output aliases an input"):
        database.select_equal_columns(
            equal.view(), database.SelectEqualPlan(equal.columns(), *columns(0, 1)), equal
        )
    with pytest.raises(ValueError, match="columns do not match the plan"):
        database.join(left_operand, right_operand, join, database.ScratchRelation(), workspace)
