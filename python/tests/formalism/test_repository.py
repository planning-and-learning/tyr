import pytest

from pytyr.formalism import datalog, planning


def test_ground_program_rule_accessors() -> None:
    assert hasattr(datalog.GroundProgram, "get_rules")
    assert hasattr(datalog.GroundProgram, "get_function_rules")
    assert not hasattr(datalog.GroundProgram, "get_ground_rules")
    assert not hasattr(datalog.GroundProgram, "get_ground_function_rules")


def test_ground_facts_expose_their_bindings() -> None:
    repository = datalog.RepositoryFactory().create_repository()

    predicate = repository.get_or_create(datalog.FluentPredicateData("predicate", 0))
    predicate_binding = repository.get_or_create(datalog.FluentPredicateBindingData(predicate, []))
    atom = repository.get_or_create(datalog.FluentGroundAtomData(predicate_binding))
    assert atom.get_row() == predicate_binding

    function = repository.get_or_create(datalog.FluentFunctionData("function", 0))
    function_binding = repository.get_or_create(datalog.FluentFunctionBindingData(function, []))
    term = repository.get_or_create(datalog.FluentGroundFunctionTermData(function_binding))
    assert term.get_row() == function_binding


@pytest.mark.parametrize("module", [datalog, planning])
def test_repository_factory_accepts_object_counts(module) -> None:
    factory = module.RepositoryFactory()
    assert factory.create_repository(num_objects=None) is not None

    parent = factory.create_repository(num_objects=2)
    parent.get_or_create(module.ObjectData("first"))
    second = parent.get_or_create(module.ObjectData("second"))
    predicate = parent.get_or_create(module.StaticPredicateData("predicate", 1))
    binding = parent.get_or_create(module.StaticPredicateBindingData(predicate, [second]))
    assert list(binding.get_objects()) == [second]

    child = factory.create_repository(parent, num_objects=3)
    third = child.get_or_create(module.ObjectData("third"))
    binding = child.get_or_create(module.StaticPredicateBindingData(predicate, [third]))
    assert list(binding.get_objects()) == [third]
