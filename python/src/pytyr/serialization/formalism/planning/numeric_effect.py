from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_expression
from pytyr.serialization.formalism.planning import function_term


class FluentNumericEffect(TypedDict):
    operator: str
    fterm: function_term.FluentFunctionTerm | str
    fexpr: function_expression.FunctionExpression | str


class AuxiliaryNumericEffect(TypedDict):
    operator: str
    fterm: function_term.AuxiliaryFunctionTerm | str
    fexpr: function_expression.FunctionExpression | str


class FluentGroundNumericEffect(TypedDict):
    operator: str
    fterm: function_term.FluentGroundFunctionTerm | str
    fexpr: function_expression.GroundFunctionExpression | str


class AuxiliaryGroundNumericEffect(TypedDict):
    operator: str
    fterm: function_term.AuxiliaryGroundFunctionTerm | str
    fexpr: function_expression.GroundFunctionExpression | str
