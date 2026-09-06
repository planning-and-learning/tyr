from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_expression_view
from pytyr.serialization.formalism.planning.function_term_view import AuxiliaryFunctionTerm, FluentFunctionTerm


class FluentNumericEffect(TypedDict):
    operator: int
    fterm: FluentFunctionTerm | str
    fexpr: function_expression_view.FunctionExpression | str


class AuxiliaryNumericEffect(TypedDict):
    operator: int
    fterm: AuxiliaryFunctionTerm | str
    fexpr: function_expression_view.FunctionExpression | str
