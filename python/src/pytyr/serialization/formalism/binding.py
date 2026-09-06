from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import function
from pytyr.serialization.formalism import object
from pytyr.serialization.formalism.planning import action
from pytyr.serialization.formalism.planning import axiom
from pytyr.serialization.formalism import predicate




class StaticPredicateBinding(TypedDict):
    relation: predicate.StaticPredicate | str
    objects: list[object.Object | str]


class FluentPredicateBinding(TypedDict):
    relation: predicate.FluentPredicate | str
    objects: list[object.Object | str]


class DerivedPredicateBinding(TypedDict):
    relation: predicate.DerivedPredicate | str
    objects: list[object.Object | str]


class StaticFunctionBinding(TypedDict):
    relation: function.StaticFunction | str
    objects: list[object.Object | str]


class FluentFunctionBinding(TypedDict):
    relation: function.FluentFunction | str
    objects: list[object.Object | str]


class AuxiliaryFunctionBinding(TypedDict):
    relation: function.AuxiliaryFunction | str
    objects: list[object.Object | str]


class ActionBinding(TypedDict):
    relation: action.Action | str
    objects: list[object.Object | str]


class AxiomBinding(TypedDict):
    relation: axiom.Axiom | str
    objects: list[object.Object | str]
