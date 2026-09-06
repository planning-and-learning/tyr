from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import numeric_effect


class FluentNumericEffectOperator(TypedDict):
    kind: str
    value: numeric_effect.FluentNumericEffect | str


class AuxiliaryNumericEffectOperator(TypedDict):
    kind: str
    value: numeric_effect.AuxiliaryNumericEffect | str


class FluentGroundNumericEffectOperator(TypedDict):
    kind: str
    value: numeric_effect.FluentGroundNumericEffect | str


class AuxiliaryGroundNumericEffectOperator(TypedDict):
    kind: str
    value: numeric_effect.AuxiliaryGroundNumericEffect | str
