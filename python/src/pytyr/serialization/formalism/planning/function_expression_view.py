from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import arithmetic_operator_view
from pytyr.serialization.formalism.planning.function_term_view import FluentFunctionTerm, StaticFunctionTerm


class FunctionExpression(TypedDict):
    kind: int
    value: float | arithmetic_operator_view.ArithmeticOperator | StaticFunctionTerm | FluentFunctionTerm | str
