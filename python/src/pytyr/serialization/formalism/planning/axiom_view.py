from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import atom_view
from pytyr.serialization.formalism.planning import conjunctive_condition_view
from pytyr.serialization.formalism import variable_view
from pytyr.serialization.formalism import binding_view


class Axiom(TypedDict):
    variables: list[variable_view.Variable | str]
    body: conjunctive_condition_view.ConjunctiveCondition | str
    head: atom_view.DerivedAtom | str


class GroundAxiom(TypedDict):
    binding: binding_view.AxiomBinding | str
    body: conjunctive_condition_view.GroundConjunctiveCondition | str
    head: atom_view.DerivedGroundAtom | str
