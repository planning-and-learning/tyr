from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.atom_view import DerivedAtom
from pytyr.serialization.formalism.planning import conjunctive_condition_view
from pytyr.serialization.formalism.variable_view import Variable


class Axiom(TypedDict):
    variables: list[Variable | str]
    body: conjunctive_condition_view.ConjunctiveCondition | str
    head: DerivedAtom | str
