from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import literal
from pytyr.serialization.formalism.planning import numeric_effect_operator
from pytyr.serialization.formalism.planning import fdr_fact


class ConjunctiveEffect(TypedDict):
    literals: list[literal.FluentLiteral | str]
    numeric_effects: list[numeric_effect_operator.FluentNumericEffectOperator | str]
    auxiliary_numeric_effect: numeric_effect_operator.AuxiliaryNumericEffectOperator | str | None


class GroundConjunctiveEffect(TypedDict):
    add_fdr_facts: list[fdr_fact.FluentFDRFact | str]
    delete_fdr_facts: list[fdr_fact.FluentFDRFact | str]
    numeric_effects: list[numeric_effect_operator.FluentGroundNumericEffectOperator | str]
    auxiliary_numeric_effect: numeric_effect_operator.AuxiliaryGroundNumericEffectOperator | str | None
