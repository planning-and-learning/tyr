from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import conditional_effect_view
from pytyr.serialization.formalism.planning import conjunctive_condition_view
from pytyr.serialization.formalism import variable_view
from pytyr.serialization.formalism import binding_view


class Action(TypedDict):
    name: str
    original_name: str
    original_arity: int
    variables: list[variable_view.Variable | str]
    condition: conjunctive_condition_view.ConjunctiveCondition | str
    effects: list[conditional_effect_view.ConditionalEffect | str]


class GroundAction(TypedDict):
    binding: binding_view.ActionBinding | str
    condition: conjunctive_condition_view.GroundConjunctiveCondition | str
    effects: list[conditional_effect_view.GroundConditionalEffect | str]
