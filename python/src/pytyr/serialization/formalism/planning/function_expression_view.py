from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import arithmetic_operator_view
from pytyr.serialization.formalism.planning import function_term_view


class FunctionExpression(TypedDict):
    kind: int
    value: float | arithmetic_operator_view.ArithmeticOperator | function_term_view.StaticFunctionTerm | function_term_view.FluentFunctionTerm | str


class GroundFunctionExpression(TypedDict):
    kind: int
    value: float | arithmetic_operator_view.GroundArithmeticOperator | function_term_view.StaticGroundFunctionTerm | function_term_view.FluentGroundFunctionTerm | function_term_view.AuxiliaryGroundFunctionTerm | str
