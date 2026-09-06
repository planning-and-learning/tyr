from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import boolean_operator_view
from pytyr.serialization.formalism.planning.literal_view import DerivedLiteral, FluentLiteral, StaticLiteral
from pytyr.serialization.formalism.variable_view import Variable


class ConjunctiveCondition(TypedDict):
    variables: list[Variable | str]
    static_literals: list[StaticLiteral | str]
    fluent_literals: list[FluentLiteral | str]
    derived_literals: list[DerivedLiteral | str]
    numeric_constraints: list[boolean_operator_view.BooleanOperator | str]
