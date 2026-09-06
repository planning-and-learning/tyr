from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import conditional_effect
from pytyr.serialization.formalism.planning import conjunctive_condition
from pytyr.serialization.formalism import variable
from pytyr.serialization.formalism import binding


class Action(TypedDict):
    name: str
    original_name: str
    original_arity: int
    variables: list[variable.Variable | str]
    condition: conjunctive_condition.ConjunctiveCondition | str
    effects: list[conditional_effect.ConditionalEffect | str]


class GroundAction(TypedDict):
    binding: binding.ActionBinding | str
    condition: conjunctive_condition.GroundConjunctiveCondition | str
    effects: list[conditional_effect.GroundConditionalEffect | str]
