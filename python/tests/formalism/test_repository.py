# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

import gc
from types import ModuleType

import pytest

from pytyr.formalism import datalog, planning


def test_ground_program_rule_accessors() -> None:
    assert hasattr(datalog.GroundProgram, "get_rules")
    assert hasattr(datalog.GroundProgram, "get_function_rules")
    assert not hasattr(datalog.GroundProgram, "get_ground_rules")
    assert not hasattr(datalog.GroundProgram, "get_ground_function_rules")


def test_ground_facts_expose_their_bindings() -> None:
    repository = datalog.Repository()

    predicate = repository.insert(datalog.FluentPredicateData("predicate", 0))[0]
    predicate_binding = repository.insert(datalog.FluentPredicateRowData(predicate, []))[0]
    atom = repository.insert(datalog.FluentGroundAtomData(predicate_binding))[0]
    assert atom.get_row() == predicate_binding

    function = repository.insert(datalog.FluentFunctionData("function", 0))[0]
    function_binding = repository.insert(datalog.FluentFunctionRowData(function, []))[0]
    term = repository.insert(datalog.FluentGroundFunctionTermData(function_binding))[0]
    assert term.get_row() == function_binding


@pytest.mark.parametrize("module", [datalog, planning])
def test_repository_accepts_object_counts(module: ModuleType) -> None:
    assert module.Repository(num_objects=None) is not None

    parent = module.Repository(num_objects=2)
    parent.insert(module.ObjectData("first"))[0]
    second = parent.insert(module.ObjectData("second"))[0]
    predicate = parent.insert(module.StaticPredicateData("predicate", 1))[0]
    binding = parent.insert(module.StaticPredicateRowData(predicate, [second]))[0]
    assert list(binding.get_objects()) == [second]

    child = module.Repository(parent, num_objects=3)
    third = child.insert(module.ObjectData("third"))[0]
    binding = child.insert(module.StaticPredicateRowData(predicate, [third]))[0]
    assert list(binding.get_objects()) == [third]


@pytest.mark.parametrize("module", [datalog, planning])
def test_insert_reports_deduplication_and_retains_repository(module: ModuleType) -> None:
    repository = module.Repository()
    data = module.ObjectData("retained")
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
    obj, _ = repository.insert(module.ObjectData("retained"))
    predicate, _ = repository.insert(module.StaticPredicateData("p", 1))
    binding, inserted = repository.insert(module.StaticPredicateRowData(predicate, [obj]))
    assert inserted is True
    duplicate, inserted = repository.insert(module.StaticPredicateRowData(predicate, [obj]))
    assert inserted is False
    assert duplicate == binding
    child = next(iter(binding.get_objects()))

    del duplicate, binding, predicate, obj, repository
    gc.collect()
    assert child.get_name() == "retained"
