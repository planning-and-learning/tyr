from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.literal_view import FluentLiteral
from pytyr.serialization.formalism.planning import numeric_effect_operator_view


class ConjunctiveEffect(TypedDict):
    literals: list[FluentLiteral | str]
    numeric_effects: list[numeric_effect_operator_view.FluentNumericEffectOperator | str]
    auxiliary_numeric_effect: numeric_effect_operator_view.AuxiliaryNumericEffectOperator | str | None
