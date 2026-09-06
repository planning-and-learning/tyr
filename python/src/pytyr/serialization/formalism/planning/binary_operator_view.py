from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_expression_view
from pytyr.serialization.formalism.planning import ground_function_expression_view


class BinaryArithmeticOperator(TypedDict):
    operator: int
    lhs: function_expression_view.FunctionExpression | str
    rhs: function_expression_view.FunctionExpression | str


class BinaryBooleanOperator(TypedDict):
    operator: int
    lhs: function_expression_view.FunctionExpression | str
    rhs: function_expression_view.FunctionExpression | str


class GroundBinaryArithmeticOperator(TypedDict):
    operator: int
    lhs: ground_function_expression_view.GroundFunctionExpression | str
    rhs: ground_function_expression_view.GroundFunctionExpression | str


class GroundBinaryBooleanOperator(TypedDict):
    operator: int
    lhs: ground_function_expression_view.GroundFunctionExpression | str
    rhs: ground_function_expression_view.GroundFunctionExpression | str
