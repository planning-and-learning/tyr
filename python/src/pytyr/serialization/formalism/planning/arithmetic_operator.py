from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import binary_operator
from pytyr.serialization.formalism.planning import multi_operator
from pytyr.serialization.formalism.planning import unary_operator




class ArithmeticOperator(TypedDict):
    kind: str
    value: unary_operator.UnaryOperator | binary_operator.BinaryArithmeticOperator | multi_operator.MultiOperator | str


class GroundArithmeticOperator(TypedDict):
    kind: str
    value: unary_operator.GroundUnaryOperator | binary_operator.GroundBinaryArithmeticOperator | multi_operator.GroundMultiOperator | str
