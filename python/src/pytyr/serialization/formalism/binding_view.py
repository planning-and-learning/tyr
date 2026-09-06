from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.function_view import AuxiliaryFunction, FluentFunction, StaticFunction
from pytyr.serialization.formalism.object_view import Object
from pytyr.serialization.formalism.planning import action_view
from pytyr.serialization.formalism.planning import axiom_view
from pytyr.serialization.formalism.predicate_view import DerivedPredicate, FluentPredicate, StaticPredicate


class StaticPredicateBinding(TypedDict):
    relation: StaticPredicate | str
    objects: list[Object | str]


class FluentPredicateBinding(TypedDict):
    relation: FluentPredicate | str
    objects: list[Object | str]


class DerivedPredicateBinding(TypedDict):
    relation: DerivedPredicate | str
    objects: list[Object | str]


class StaticFunctionBinding(TypedDict):
    relation: StaticFunction | str
    objects: list[Object | str]


class FluentFunctionBinding(TypedDict):
    relation: FluentFunction | str
    objects: list[Object | str]


class AuxiliaryFunctionBinding(TypedDict):
    relation: AuxiliaryFunction | str
    objects: list[Object | str]


class ActionBinding(TypedDict):
    relation: action_view.Action | str
    objects: list[Object | str]


class AxiomBinding(TypedDict):
    relation: axiom_view.Axiom | str
    objects: list[Object | str]
