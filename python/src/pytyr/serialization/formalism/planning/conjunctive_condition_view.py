from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import boolean_operator_view
from pytyr.serialization.formalism.planning import literal_view
from pytyr.serialization.formalism import variable_view
from pytyr.serialization.formalism.planning import fdr_fact_view


class ConjunctiveCondition(TypedDict):
    variables: list[variable_view.Variable | str]
    static_literals: list[literal_view.StaticLiteral | str]
    fluent_literals: list[literal_view.FluentLiteral | str]
    derived_literals: list[literal_view.DerivedLiteral | str]
    numeric_constraints: list[boolean_operator_view.BooleanOperator | str]


class GroundConjunctiveCondition(TypedDict):
    static_literals: list[literal_view.StaticGroundLiteral | str]
    derived_literals: list[literal_view.DerivedGroundLiteral | str]
    positive_facts: list[fdr_fact_view.FluentFDRFact | str]
    negative_facts: list[fdr_fact_view.FluentFDRFact | str]
    numeric_constraints: list[boolean_operator_view.GroundBooleanOperator | str]
