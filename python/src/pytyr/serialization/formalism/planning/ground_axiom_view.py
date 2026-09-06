from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.binding_view import AxiomBinding
from pytyr.serialization.formalism.planning.ground_atom_view import DerivedGroundAtom
from pytyr.serialization.formalism.planning.ground_conjunctive_condition_view import GroundConjunctiveCondition


class GroundAxiom(TypedDict):
    binding: AxiomBinding | str
    body: GroundConjunctiveCondition | str
    head: DerivedGroundAtom | str
