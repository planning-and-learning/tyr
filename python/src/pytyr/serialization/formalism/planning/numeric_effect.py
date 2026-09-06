from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_expression
from pytyr.serialization.formalism.planning import function_term


class FluentNumericEffect(TypedDict):
    operator: str
    function_term: function_term.FluentFunctionTerm | str
    function_expression: function_expression.FunctionExpression | str


class AuxiliaryNumericEffect(TypedDict):
    operator: str
    function_term: function_term.AuxiliaryFunctionTerm | str
    function_expression: function_expression.FunctionExpression | str


class FluentGroundNumericEffect(TypedDict):
    operator: str
    function_term: function_term.FluentGroundFunctionTerm | str
    function_expression: function_expression.GroundFunctionExpression | str


class AuxiliaryGroundNumericEffect(TypedDict):
    operator: str
    function_term: function_term.AuxiliaryGroundFunctionTerm | str
    function_expression: function_expression.GroundFunctionExpression | str
