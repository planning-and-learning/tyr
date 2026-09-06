from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.binding_view import ActionBinding
from pytyr.serialization.formalism.planning.ground_conditional_effect_view import GroundConditionalEffect
from pytyr.serialization.formalism.planning.ground_conjunctive_condition_view import GroundConjunctiveCondition


class GroundAction(TypedDict):
    binding: ActionBinding | str
    condition: GroundConjunctiveCondition | str
    effects: list[GroundConditionalEffect | str]
