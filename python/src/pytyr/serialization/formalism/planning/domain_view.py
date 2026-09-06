from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import function_view
from pytyr.serialization.formalism import object_view
from pytyr.serialization.formalism.planning import action_view
from pytyr.serialization.formalism.planning import axiom_view
from pytyr.serialization.formalism import predicate_view




class Domain(TypedDict):
    name: str
    static_predicates: list[predicate_view.StaticPredicate | str]
    fluent_predicates: list[predicate_view.FluentPredicate | str]
    derived_predicates: list[predicate_view.DerivedPredicate | str]
    static_functions: list[function_view.StaticFunction | str]
    fluent_functions: list[function_view.FluentFunction | str]
    auxiliary_function: function_view.AuxiliaryFunction | str | None
    constants: list[object_view.Object | str]
    actions: list[action_view.Action | str]
    axioms: list[axiom_view.Axiom | str]
