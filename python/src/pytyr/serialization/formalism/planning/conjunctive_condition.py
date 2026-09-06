from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import boolean_operator
from pytyr.serialization.formalism.planning import literal
from pytyr.serialization.formalism import variable
from pytyr.serialization.formalism.planning import fdr_fact


class ConjunctiveCondition(TypedDict):
    variables: list[variable.Variable | str]
    static_literals: list[literal.StaticLiteral | str]
    fluent_literals: list[literal.FluentLiteral | str]
    derived_literals: list[literal.DerivedLiteral | str]
    numeric_constraints: list[boolean_operator.BooleanOperator | str]


class GroundConjunctiveCondition(TypedDict):
    static_literals: list[literal.StaticGroundLiteral | str]
    derived_literals: list[literal.DerivedGroundLiteral | str]
    positive_facts: list[fdr_fact.FluentFDRFact | str]
    negative_facts: list[fdr_fact.FluentFDRFact | str]
    numeric_constraints: list[boolean_operator.GroundBooleanOperator | str]
