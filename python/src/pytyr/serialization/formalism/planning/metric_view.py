from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_expression_view




class Metric(TypedDict):
    optimization_direction: str
    fexpr: function_expression_view.GroundFunctionExpression | str
