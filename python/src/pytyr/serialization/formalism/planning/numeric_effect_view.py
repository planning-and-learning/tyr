from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_expression_view
from pytyr.serialization.formalism.planning import function_term_view


class FluentNumericEffect(TypedDict):
    operator: str
    fterm: function_term_view.FluentFunctionTerm | str
    fexpr: function_expression_view.FunctionExpression | str


class AuxiliaryNumericEffect(TypedDict):
    operator: str
    fterm: function_term_view.AuxiliaryFunctionTerm | str
    fexpr: function_expression_view.FunctionExpression | str


class FluentGroundNumericEffect(TypedDict):
    operator: str
    fterm: function_term_view.FluentGroundFunctionTerm | str
    fexpr: function_expression_view.GroundFunctionExpression | str


class AuxiliaryGroundNumericEffect(TypedDict):
    operator: str
    fterm: function_term_view.AuxiliaryGroundFunctionTerm | str
    fexpr: function_expression_view.GroundFunctionExpression | str
