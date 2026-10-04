import gc

import pytest

from pytyr.formalism import datalog, planning


def test_ground_program_rule_accessors() -> None:
    assert hasattr(datalog.GroundProgram, "get_rules")
    assert hasattr(datalog.GroundProgram, "get_function_rules")
    assert not hasattr(datalog.GroundProgram, "get_ground_rules")
    assert not hasattr(datalog.GroundProgram, "get_ground_function_rules")


def test_ground_facts_expose_their_bindings() -> None:
    repository = datalog.RepositoryFactory().create_repository()

    predicate = repository.insert(datalog.FluentPredicateData("predicate", 0))[0]
    predicate_binding = repository.insert(datalog.FluentPredicateBindingData(predicate, []))[0]
    atom = repository.insert(datalog.FluentGroundAtomData(predicate_binding))[0]
    assert atom.get_row() == predicate_binding

    function = repository.insert(datalog.FluentFunctionData("function", 0))[0]
    function_binding = repository.insert(datalog.FluentFunctionBindingData(function, []))[0]
    term = repository.insert(datalog.FluentGroundFunctionTermData(function_binding))[0]
    assert term.get_row() == function_binding


@pytest.mark.parametrize("module", [datalog, planning])
def test_repository_factory_accepts_object_counts(module) -> None:
    factory = module.RepositoryFactory()
    assert factory.create_repository(num_objects=None) is not None

    parent = factory.create_repository(num_objects=2)
    parent.insert(module.ObjectData("first"))[0]
    second = parent.insert(module.ObjectData("second"))[0]
    predicate = parent.insert(module.StaticPredicateData("predicate", 1))[0]
    binding = parent.insert(module.StaticPredicateBindingData(predicate, [second]))[0]
    assert list(binding.get_objects()) == [second]

    child = factory.create_repository(parent, num_objects=3)
    third = child.insert(module.ObjectData("third"))[0]
    binding = child.insert(module.StaticPredicateBindingData(predicate, [third]))[0]
    assert list(binding.get_objects()) == [third]


@pytest.mark.parametrize("module", [datalog, planning])
def test_insert_reports_deduplication_and_retains_repository(module) -> None:
    factory = module.RepositoryFactory()
    repository = factory.create_repository()
    data = module.ObjectData("retained")
    value, inserted = repository.insert(data)
    assert inserted is True
    duplicate, inserted = repository.insert(data)
    assert inserted is False
    assert duplicate == value
    assert not hasattr(repository, "get_or_create")

    del duplicate, data, repository, factory
    gc.collect()
    assert value.get_name() == "retained"


@pytest.mark.parametrize("module", [datalog, planning])
def test_inserted_binding_children_retain_repository(module) -> None:
    repository = module.RepositoryFactory().create_repository()
    obj, _ = repository.insert(module.ObjectData("retained"))
    predicate, _ = repository.insert(module.StaticPredicateData("p", 1))
    binding, inserted = repository.insert(module.StaticPredicateBindingData(predicate, [obj]))
    assert inserted is True
    duplicate, inserted = repository.insert(module.StaticPredicateBindingData(predicate, [obj]))
    assert inserted is False
    assert duplicate == binding
    child = next(iter(binding.get_objects()))

    del duplicate, binding, predicate, obj, repository
    gc.collect()
    assert child.get_name() == "retained"
