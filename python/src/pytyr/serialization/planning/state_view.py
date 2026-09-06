from __future__ import annotations

from typing import TypedDict
from ..formalism.planning.fdr_fact_view import FluentFDRFact
from ..formalism.planning.atom_view import DerivedGroundAtom
from ..formalism.planning.function_term_view import FluentGroundFunctionTerm




class State(TypedDict):
    fluent_facts: list[FluentFDRFact | str]
    derived_atoms: list[DerivedGroundAtom | str]
    fluent_fterm_values: list[list[FluentGroundFunctionTerm | str | float]]
