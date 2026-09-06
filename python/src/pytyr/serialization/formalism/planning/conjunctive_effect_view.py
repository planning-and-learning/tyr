from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import literal_view
from pytyr.serialization.formalism.planning import numeric_effect_operator_view
from pytyr.serialization.formalism.planning import fdr_fact_view


class ConjunctiveEffect(TypedDict):
    literals: list[literal_view.FluentLiteral | str]
    numeric_effects: list[numeric_effect_operator_view.FluentNumericEffectOperator | str]
    auxiliary_numeric_effect: numeric_effect_operator_view.AuxiliaryNumericEffectOperator | str | None


class GroundConjunctiveEffect(TypedDict):
    add_facts: list[fdr_fact_view.FluentFDRFact | str]
    del_facts: list[fdr_fact_view.FluentFDRFact | str]
    numeric_effects: list[numeric_effect_operator_view.FluentGroundNumericEffectOperator | str]
    auxiliary_numeric_effect: numeric_effect_operator_view.AuxiliaryGroundNumericEffectOperator | str | None
