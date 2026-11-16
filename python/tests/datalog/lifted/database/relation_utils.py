# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Reading and writing relations by decoded values; interning free relations through their records."""

from collections.abc import Iterable, Sequence
from typing import Any

from pyyggdrasil import formalism


def labels(columns: Iterable[formalism.ColumnLayout]) -> list[formalism.ColumnIndex]:
    """The labels of `columns` (a relation's or a plan's)."""
    return [column.label for column in columns]


def insert(builder: formalism.RelationBuilder, values: Sequence[Any]) -> int:
    """Inserts `values`, each encoded by the type of its column."""
    return builder.insert([formalism.encode(v, c.type) for v, c in zip(values, builder.columns(), strict=True)])


# `row` is a row of a builder (RowBuilderView) or of an interned relation (RowSchema); the bindings give them no common
# base.
def values(row: Any) -> tuple[object, ...]:  # noqa: ANN401
    """The decoded values of `row`."""
    return tuple(row.get(i) for i in range(row.size()))


# `relation` is a relation view (RelationBuilderView) or an interned relation (RelationSchema).
def tuples(relation: Any) -> list[tuple[object, ...]]:  # noqa: ANN401
    """The decoded rows of `relation`, in its order."""
    return [values(row) for row in relation.get_rows()]


# `relation` is any of the forms the docstring names; the bindings give them no common base.
def intern(repository: formalism.RelationRepository, relation: Any) -> tuple[formalism.RelationSchema, bool]:  # noqa: ANN401
    """Interns `relation` (a builder, a borrowed or an interned relation of any repository) in `repository`."""
    if isinstance(relation, formalism.RelationBuilder):
        relation = repository.view(relation)
    columns = relation.columns()
    pool = repository.schema([column.type for column in columns])
    rows = [repository.insert(formalism.RowSchemaData(pool, row.get_values()))[0] for row in relation.get_rows()]
    layout = repository.insert(formalism.ColumnsData(columns))[0]
    return repository.insert(formalism.RelationSchemaData(pool, layout.index(), [each.index() for each in rows]))


def rename(
    repository: formalism.RelationRepository, relation: formalism.RelationSchema, new: Sequence[formalism.ColumnIndex]
) -> formalism.RelationSchema:
    """`relation` (of `repository`) under new labels: the same pool and rows."""
    columns = relation.columns()
    if len(new) != len(columns):
        message = "Columns: renaming must preserve arity."
        raise ValueError(message)
    layout = [formalism.ColumnLayout(label, column.type) for label, column in zip(new, columns, strict=True)]
    renamed = repository.insert(formalism.ColumnsData(layout))[0]
    return repository.insert(
        formalism.RelationSchemaData(
            relation.get_pool().index(), renamed.index(), [each.index() for each in relation.get_rows()]
        )
    )[0]
