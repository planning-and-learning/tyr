from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import atom
from pytyr.serialization.formalism.planning import conjunctive_condition
from pytyr.serialization.formalism import variable
from pytyr.serialization.formalism import binding


class Axiom(TypedDict):
    variables: list[variable.Variable | str]
    body: conjunctive_condition.ConjunctiveCondition | str
    head: atom.DerivedAtom | str


class GroundAxiom(TypedDict):
    binding: binding.AxiomBinding | str
    body: conjunctive_condition.GroundConjunctiveCondition | str
    head: atom.DerivedGroundAtom | str
