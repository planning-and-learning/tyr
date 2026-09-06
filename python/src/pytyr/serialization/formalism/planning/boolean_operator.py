from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import binary_operator




class BooleanOperator(TypedDict):
    kind: str
    value: binary_operator.BinaryBooleanOperator | str


class GroundBooleanOperator(TypedDict):
    kind: str
    value: binary_operator.GroundBinaryBooleanOperator | str
