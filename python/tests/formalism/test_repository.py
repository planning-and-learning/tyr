# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
from types import ModuleType

import pytest

from pytyr import formalism
from pytyr.formalism import datalog, planning


def test_ground_program_rule_accessors() -> None:
    assert hasattr(datalog.ProgramGround, "get_rules")
    assert not hasattr(datalog.ProgramGround, "get_ground_rules")


def test_ground_facts_expose_their_bindings() -> None:
    repository = datalog.Repository()

    predicate = repository.insert(formalism.PredicateFluentData("predicate", 0))[0]
    predicate_binding = repository.insert(formalism.RowPredicateFluentData(predicate, []))[0]
    atom = repository.insert(formalism.AtomGroundFluentData(predicate_binding))[0]
    assert atom.get_row() == predicate_binding

    function = repository.insert(formalism.FunctionFluentData("function", 0))[0]
    function_binding = repository.insert(formalism.RowFunctionFluentData(function, []))[0]
    term = repository.insert(formalism.FunctionTermGroundFluentData(function_binding))[0]
    assert term.get_row() == function_binding


@pytest.mark.parametrize("module", [datalog, planning])
def test_repository_accepts_object_counts(module: ModuleType) -> None:
    assert module.Repository(num_objects=None) is not None

    parent = module.Repository(num_objects=2)
    parent.insert(formalism.ObjectData("first"))[0]
    second = parent.insert(formalism.ObjectData("second"))[0]
    predicate = parent.insert(formalism.PredicateStaticData("predicate", 1))[0]
    binding = parent.insert(formalism.RowPredicateStaticData(predicate, [second.index()]))[0]
    assert list(binding.get_values()) == [second.index().value]

    child = module.Repository(parent, num_objects=3)
    third = child.insert(formalism.ObjectData("third"))[0]
    binding = child.insert(formalism.RowPredicateStaticData(predicate, [third.index()]))[0]
    assert list(binding.get_values()) == [third.index().value]


@pytest.mark.parametrize("module", [datalog, planning])
def test_insert_reports_deduplication_and_retains_repository(module: ModuleType) -> None:
    repository = module.Repository()
    data = formalism.ObjectData("retained")
    value, inserted = repository.insert(data)
    assert inserted is True
    duplicate, inserted = repository.insert(data)
    assert inserted is False
    assert duplicate == value
    assert not hasattr(repository, "get_or_create")

    del duplicate, data, repository
    gc.collect()
    assert value.get_name() == "retained"


@pytest.mark.parametrize("module", [datalog, planning])
def test_inserted_binding_children_retain_repository(module: ModuleType) -> None:
    repository = module.Repository()
    obj, _ = repository.insert(formalism.ObjectData("retained"))
    predicate, _ = repository.insert(formalism.PredicateStaticData("p", 1))
    binding, inserted = repository.insert(formalism.RowPredicateStaticData(predicate, [obj.index()]))
    assert inserted is True
    duplicate, inserted = repository.insert(formalism.RowPredicateStaticData(predicate, [obj.index()]))
    assert inserted is False
    assert duplicate == binding
    child = binding.get_relation()  # a child view of the row: its predicate

    del duplicate, binding, predicate, obj, repository
    gc.collect()
    assert child.get_name() == "p"
