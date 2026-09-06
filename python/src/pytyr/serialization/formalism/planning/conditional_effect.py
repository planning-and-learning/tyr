from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import conjunctive_condition
from pytyr.serialization.formalism.planning import conjunctive_effect
from pytyr.serialization.formalism import variable


class ConditionalEffect(TypedDict):
    variables: list[variable.Variable | str]
    condition: conjunctive_condition.ConjunctiveCondition | str
    effect: conjunctive_effect.ConjunctiveEffect | str


class GroundConditionalEffect(TypedDict):
    condition: conjunctive_condition.GroundConjunctiveCondition | str
    effect: conjunctive_effect.GroundConjunctiveEffect | str
