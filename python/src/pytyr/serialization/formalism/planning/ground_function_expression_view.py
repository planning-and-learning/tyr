from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import arithmetic_operator_view
from pytyr.serialization.formalism.planning import ground_function_term_view


class GroundFunctionExpression(TypedDict):
    kind: int
    value: float | arithmetic_operator_view.GroundArithmeticOperator | ground_function_term_view.StaticGroundFunctionTerm | ground_function_term_view.FluentGroundFunctionTerm | ground_function_term_view.AuxiliaryGroundFunctionTerm | str
