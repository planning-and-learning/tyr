from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.ground_function_expression_view import GroundFunctionExpression
from pytyr.serialization.formalism.planning.ground_function_term_view import AuxiliaryGroundFunctionTerm, FluentGroundFunctionTerm


class FluentGroundNumericEffect(TypedDict):
    operator: int
    fterm: FluentGroundFunctionTerm | str
    fexpr: GroundFunctionExpression | str


class AuxiliaryGroundNumericEffect(TypedDict):
    operator: int
    fterm: AuxiliaryGroundFunctionTerm | str
    fexpr: GroundFunctionExpression | str
