from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_expression_view




class UnaryOperator(TypedDict):
    operator: str
    arg: function_expression_view.FunctionExpression | str


class GroundUnaryOperator(TypedDict):
    operator: str
    arg: function_expression_view.GroundFunctionExpression | str
