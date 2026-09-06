from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import numeric_effect_view


class FluentNumericEffectOperator(TypedDict):
    kind: int
    value: numeric_effect_view.FluentNumericEffect | str


class AuxiliaryNumericEffectOperator(TypedDict):
    kind: int
    value: numeric_effect_view.AuxiliaryNumericEffect | str
