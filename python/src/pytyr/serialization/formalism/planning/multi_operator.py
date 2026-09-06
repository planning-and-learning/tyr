from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_expression




class MultiOperator(TypedDict):
    operator: str
    args: list[function_expression.FunctionExpression | str]


class GroundMultiOperator(TypedDict):
    operator: str
    args: list[function_expression.GroundFunctionExpression | str]
