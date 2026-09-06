from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.fdr_fact_view import FluentFDRFact
from pytyr.serialization.formalism.planning.ground_numeric_effect_operator_view import AuxiliaryGroundNumericEffectOperator, FluentGroundNumericEffectOperator


class GroundConjunctiveEffect(TypedDict):
    add_facts: list[FluentFDRFact | str]
    del_facts: list[FluentFDRFact | str]
    numeric_effects: list[FluentGroundNumericEffectOperator | str]
    auxiliary_numeric_effect: AuxiliaryGroundNumericEffectOperator | str | None
