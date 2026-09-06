from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_expression




class UnaryOperator(TypedDict):
    operator: str
    arg: function_expression.FunctionExpression | str


class GroundUnaryOperator(TypedDict):
    operator: str
    arg: function_expression.GroundFunctionExpression | str
