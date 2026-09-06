from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.ground_conjunctive_condition_view import GroundConjunctiveCondition
from pytyr.serialization.formalism.planning.ground_conjunctive_effect_view import GroundConjunctiveEffect


class GroundConditionalEffect(TypedDict):
    condition: GroundConjunctiveCondition | str
    effect: GroundConjunctiveEffect | str
