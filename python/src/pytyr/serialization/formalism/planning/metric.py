from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_expression




class Metric(TypedDict):
    optimization_direction: str
    fexpr: function_expression.GroundFunctionExpression | str
