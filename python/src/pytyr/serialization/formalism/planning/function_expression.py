from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import arithmetic_operator
from pytyr.serialization.formalism.planning import function_term


class FunctionExpression(TypedDict):
    kind: str
    value: float | arithmetic_operator.ArithmeticOperator | function_term.StaticFunctionTerm | function_term.FluentFunctionTerm | str


class GroundFunctionExpression(TypedDict):
    kind: str
    value: float | arithmetic_operator.GroundArithmeticOperator | function_term.StaticGroundFunctionTerm | function_term.FluentGroundFunctionTerm | function_term.AuxiliaryGroundFunctionTerm | str
