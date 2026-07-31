import pytest

from pytyr.formalism import datalog, planning


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
