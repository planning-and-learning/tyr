from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import object
from pytyr.serialization.formalism.planning import axiom
from pytyr.serialization.formalism.planning import domain
from pytyr.serialization.formalism.planning import fdr_fact
from pytyr.serialization.formalism.planning import fdr_variable
from pytyr.serialization.formalism.planning import action
from pytyr.serialization.formalism.planning import atom
from pytyr.serialization.formalism.planning import conjunctive_condition
from pytyr.serialization.formalism.planning import function_term_value
from pytyr.serialization.formalism.planning import metric
from pytyr.serialization.formalism import predicate




class GroundTask(TypedDict):
    name: str
    domain: domain.Domain | str
    derived_predicates: list[predicate.DerivedPredicate | str]
    objects: list[object.Object | str]
    static_ground_atoms: list[atom.StaticGroundAtom | str]
    fluent_ground_atoms: list[atom.FluentGroundAtom | str]
    static_ground_function_term_values: list[function_term_value.StaticGroundFunctionTermValue | str]
    fluent_ground_function_term_values: list[function_term_value.FluentGroundFunctionTermValue | str]
    auxiliary_ground_function_term_value: function_term_value.AuxiliaryGroundFunctionTermValue | str | None
    goal: conjunctive_condition.GroundConjunctiveCondition | str
    metric: metric.Metric | str | None
    axioms: list[axiom.Axiom | str]
    derived_ground_atoms: list[atom.DerivedGroundAtom | str]
    fdr_variables: list[fdr_variable.FluentFDRVariable | str]
    fdr_facts: list[fdr_fact.FluentFDRFact | str]
    ground_actions: list[action.GroundAction | str]
    ground_axioms: list[axiom.GroundAxiom | str]
