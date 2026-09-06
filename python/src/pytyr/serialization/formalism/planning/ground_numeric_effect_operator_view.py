from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.ground_numeric_effect_view import AuxiliaryGroundNumericEffect, FluentGroundNumericEffect


class FluentGroundNumericEffectOperator(TypedDict):
    kind: int
    value: FluentGroundNumericEffect | str


class AuxiliaryGroundNumericEffectOperator(TypedDict):
    kind: int
    value: AuxiliaryGroundNumericEffect | str
