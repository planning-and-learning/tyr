from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.function_view import AuxiliaryFunction, FluentFunction, StaticFunction
from pytyr.serialization.formalism.object_view import Object
from pytyr.serialization.formalism.planning.action_view import Action
from pytyr.serialization.formalism.planning.axiom_view import Axiom
from pytyr.serialization.formalism.predicate_view import DerivedPredicate, FluentPredicate, StaticPredicate


class Domain(TypedDict):
    name: str
    static_predicates: list[StaticPredicate | str]
    fluent_predicates: list[FluentPredicate | str]
    derived_predicates: list[DerivedPredicate | str]
    static_functions: list[StaticFunction | str]
    fluent_functions: list[FluentFunction | str]
    auxiliary_function: AuxiliaryFunction | str | None
    constants: list[Object | str]
    actions: list[Action | str]
    axioms: list[Axiom | str]
