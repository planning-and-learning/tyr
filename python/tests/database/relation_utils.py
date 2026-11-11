# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

"""Interning free relations through their records: rows, then columns, then the relation."""

from collections.abc import Iterable
from typing import Any

from pyyggdrasil import formalism


# `columns` is any of the forms the bindings hand out a relation's columns in (a builder's, a view's).
def _layouts(columns: Any, labels: Iterable[formalism.ColumnIndex] | None = None) -> list[formalism.ColumnLayout]:  # noqa: ANN401
    names = list(columns) if labels is None else list(labels)
    if len(names) != len(columns):
        message = "Columns: renaming must preserve arity."
        raise ValueError(message)
    return [formalism.ColumnLayout(names[i], columns.type(i)) for i in range(len(columns))]


# `relation` is any of the forms the docstring names; the bindings give them no common base.
def intern(repository: formalism.RelationRepository, relation: Any) -> tuple[formalism.RelationView, bool]:  # noqa: ANN401
    """Interns `relation` (a builder, a borrowed or an interned relation of any repository) in `repository`."""
    if isinstance(relation, formalism.Relation):
        relation = repository.view(relation)
    columns = relation.columns()
    pool = repository.schema([columns.type(i) for i in range(len(columns))])
    rows = [repository.insert(formalism.RowData(pool, list(row)))[0] for row in relation]
    labels = repository.insert(formalism.ColumnsData(_layouts(columns)))[0]
    return repository.insert(formalism.RelationData(pool, labels, rows))


def rename(
    repository: formalism.RelationRepository, relation: formalism.RelationView, labels: Iterable[formalism.ColumnIndex]
) -> formalism.RelationView:
    """`relation` (of `repository`) under new labels: the same pool and rows."""
    data = relation.data()
    data.columns = repository.insert(formalism.ColumnsData(_layouts(relation.columns(), labels)))[0]
    return repository.insert(data)[0]
