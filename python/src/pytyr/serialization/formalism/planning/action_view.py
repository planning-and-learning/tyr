from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import conditional_effect_view
from pytyr.serialization.formalism.planning import conjunctive_condition_view
from pytyr.serialization.formalism.variable_view import Variable


class Action(TypedDict):
    name: str
    original_name: str
    original_arity: int
    variables: list[Variable | str]
    condition: conjunctive_condition_view.ConjunctiveCondition | str
    effects: list[conditional_effect_view.ConditionalEffect | str]
