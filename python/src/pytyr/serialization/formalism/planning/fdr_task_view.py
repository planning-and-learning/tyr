from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import object_view
from pytyr.serialization.formalism.planning import axiom_view
from pytyr.serialization.formalism.planning import domain_view
from pytyr.serialization.formalism.planning import fdr_fact_view
from pytyr.serialization.formalism.planning import fdr_variable_view
from pytyr.serialization.formalism.planning import action_view
from pytyr.serialization.formalism.planning import atom_view
from pytyr.serialization.formalism.planning import conjunctive_condition_view
from pytyr.serialization.formalism.planning import function_term_value_view
from pytyr.serialization.formalism.planning import metric_view
from pytyr.serialization.formalism import predicate_view




class GroundTask(TypedDict):
    name: str
    domain: domain_view.Domain | str
    derived_predicates: list[predicate_view.DerivedPredicate | str]
    objects: list[object_view.Object | str]
    static_atoms: list[atom_view.StaticGroundAtom | str]
    fluent_atoms: list[atom_view.FluentGroundAtom | str]
    static_fterm_values: list[function_term_value_view.StaticGroundFunctionTermValue | str]
    fluent_fterm_values: list[function_term_value_view.FluentGroundFunctionTermValue | str]
    auxiliary_fterm_value: function_term_value_view.AuxiliaryGroundFunctionTermValue | str | None
    goal: conjunctive_condition_view.GroundConjunctiveCondition | str
    metric: metric_view.Metric | str | None
    axioms: list[axiom_view.Axiom | str]
    derived_atoms: list[atom_view.DerivedGroundAtom | str]
    fluent_variables: list[fdr_variable_view.FluentFDRVariable | str]
    fluent_facts: list[fdr_fact_view.FluentFDRFact | str]
    ground_actions: list[action_view.GroundAction | str]
    ground_axioms: list[axiom_view.GroundAxiom | str]
