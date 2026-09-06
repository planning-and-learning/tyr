from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import function_view
from pytyr.serialization.formalism import object_view
from pytyr.serialization.formalism.planning import action_view
from pytyr.serialization.formalism.planning import axiom_view
from pytyr.serialization.formalism import predicate_view




class StaticPredicateBinding(TypedDict):
    relation: predicate_view.StaticPredicate | str
    objects: list[object_view.Object | str]


class FluentPredicateBinding(TypedDict):
    relation: predicate_view.FluentPredicate | str
    objects: list[object_view.Object | str]


class DerivedPredicateBinding(TypedDict):
    relation: predicate_view.DerivedPredicate | str
    objects: list[object_view.Object | str]


class StaticFunctionBinding(TypedDict):
    relation: function_view.StaticFunction | str
    objects: list[object_view.Object | str]


class FluentFunctionBinding(TypedDict):
    relation: function_view.FluentFunction | str
    objects: list[object_view.Object | str]


class AuxiliaryFunctionBinding(TypedDict):
    relation: function_view.AuxiliaryFunction | str
    objects: list[object_view.Object | str]


class ActionBinding(TypedDict):
    relation: action_view.Action | str
    objects: list[object_view.Object | str]


class AxiomBinding(TypedDict):
    relation: axiom_view.Axiom | str
    objects: list[object_view.Object | str]
