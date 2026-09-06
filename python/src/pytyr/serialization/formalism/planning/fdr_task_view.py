from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.object_view import Object
from pytyr.serialization.formalism.planning.axiom_view import Axiom
from pytyr.serialization.formalism.planning.domain_view import Domain
from pytyr.serialization.formalism.planning.fdr_fact_view import FluentFDRFact
from pytyr.serialization.formalism.planning.fdr_variable_view import FluentFDRVariable
from pytyr.serialization.formalism.planning.ground_action_view import GroundAction
from pytyr.serialization.formalism.planning.ground_atom_view import DerivedGroundAtom, FluentGroundAtom, StaticGroundAtom
from pytyr.serialization.formalism.planning.ground_axiom_view import GroundAxiom
from pytyr.serialization.formalism.planning.ground_conjunctive_condition_view import GroundConjunctiveCondition
from pytyr.serialization.formalism.planning.ground_function_term_value_view import AuxiliaryGroundFunctionTermValue, FluentGroundFunctionTermValue, StaticGroundFunctionTermValue
from pytyr.serialization.formalism.planning.metric_view import Metric
from pytyr.serialization.formalism.predicate_view import DerivedPredicate


class GroundTask(TypedDict):
    name: str
    domain: Domain | str
    derived_predicates: list[DerivedPredicate | str]
    objects: list[Object | str]
    static_atoms: list[StaticGroundAtom | str]
    fluent_atoms: list[FluentGroundAtom | str]
    static_fterm_values: list[StaticGroundFunctionTermValue | str]
    fluent_fterm_values: list[FluentGroundFunctionTermValue | str]
    auxiliary_fterm_value: AuxiliaryGroundFunctionTermValue | str | None
    goal: GroundConjunctiveCondition | str
    metric: Metric | str | None
    axioms: list[Axiom | str]
    derived_atoms: list[DerivedGroundAtom | str]
    fluent_variables: list[FluentFDRVariable | str]
    fluent_facts: list[FluentFDRFact | str]
    ground_actions: list[GroundAction | str]
    ground_axioms: list[GroundAxiom | str]
