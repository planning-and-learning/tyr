from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.ground_function_expression_view import GroundFunctionExpression


class Metric(TypedDict):
    optimization_direction: int
    fexpr: GroundFunctionExpression | str
