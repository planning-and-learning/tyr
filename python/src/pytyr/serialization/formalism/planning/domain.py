from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import function
from pytyr.serialization.formalism import object
from pytyr.serialization.formalism.planning import action
from pytyr.serialization.formalism.planning import axiom
from pytyr.serialization.formalism import predicate




class Domain(TypedDict):
    name: str
    static_predicates: list[predicate.StaticPredicate | str]
    fluent_predicates: list[predicate.FluentPredicate | str]
    derived_predicates: list[predicate.DerivedPredicate | str]
    static_functions: list[function.StaticFunction | str]
    fluent_functions: list[function.FluentFunction | str]
    auxiliary_function: function.AuxiliaryFunction | str | None
    constants: list[object.Object | str]
    actions: list[action.Action | str]
    axioms: list[axiom.Axiom | str]
