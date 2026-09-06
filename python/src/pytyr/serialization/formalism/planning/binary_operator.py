from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_expression




class BinaryArithmeticOperator(TypedDict):
    operator: str
    lhs: function_expression.FunctionExpression | str
    rhs: function_expression.FunctionExpression | str


class BinaryBooleanOperator(TypedDict):
    operator: str
    lhs: function_expression.FunctionExpression | str
    rhs: function_expression.FunctionExpression | str


class GroundBinaryArithmeticOperator(TypedDict):
    operator: str
    lhs: function_expression.GroundFunctionExpression | str
    rhs: function_expression.GroundFunctionExpression | str


class GroundBinaryBooleanOperator(TypedDict):
    operator: str
    lhs: function_expression.GroundFunctionExpression | str
    rhs: function_expression.GroundFunctionExpression | str
