from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import binary_operator_view
from pytyr.serialization.formalism.planning import multi_operator_view
from pytyr.serialization.formalism.planning import unary_operator_view




class ArithmeticOperator(TypedDict):
    kind: str
    value: unary_operator_view.UnaryOperator | binary_operator_view.BinaryArithmeticOperator | multi_operator_view.MultiOperator | str


class GroundArithmeticOperator(TypedDict):
    kind: str
    value: unary_operator_view.GroundUnaryOperator | binary_operator_view.GroundBinaryArithmeticOperator | multi_operator_view.GroundMultiOperator | str
