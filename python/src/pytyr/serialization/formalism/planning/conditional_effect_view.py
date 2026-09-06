from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import conjunctive_condition_view
from pytyr.serialization.formalism.planning import conjunctive_effect_view
from pytyr.serialization.formalism.variable_view import Variable


class ConditionalEffect(TypedDict):
    variables: list[Variable | str]
    condition: conjunctive_condition_view.ConjunctiveCondition | str
    effect: conjunctive_effect_view.ConjunctiveEffect | str
