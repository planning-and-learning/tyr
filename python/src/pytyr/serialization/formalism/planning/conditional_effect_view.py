from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import conjunctive_condition_view
from pytyr.serialization.formalism.planning import conjunctive_effect_view
from pytyr.serialization.formalism import variable_view


class ConditionalEffect(TypedDict):
    variables: list[variable_view.Variable | str]
    condition: conjunctive_condition_view.ConjunctiveCondition | str
    effect: conjunctive_effect_view.ConjunctiveEffect | str


class GroundConditionalEffect(TypedDict):
    condition: conjunctive_condition_view.GroundConjunctiveCondition | str
    effect: conjunctive_effect_view.GroundConjunctiveEffect | str
