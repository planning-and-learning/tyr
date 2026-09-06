from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.boolean_operator_view import GroundBooleanOperator
from pytyr.serialization.formalism.planning.fdr_fact_view import FluentFDRFact
from pytyr.serialization.formalism.planning.ground_literal_view import DerivedGroundLiteral, StaticGroundLiteral


class GroundConjunctiveCondition(TypedDict):
    static_literals: list[StaticGroundLiteral | str]
    derived_literals: list[DerivedGroundLiteral | str]
    positive_facts: list[FluentFDRFact | str]
    negative_facts: list[FluentFDRFact | str]
    numeric_constraints: list[GroundBooleanOperator | str]
