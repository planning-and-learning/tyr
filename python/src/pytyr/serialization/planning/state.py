from typing import TypedDict
from ..formalism.planning.atom import DerivedGroundAtom, FluentGroundAtom
from ..formalism.planning.function_term import FluentGroundFunctionTerm




class State(TypedDict):
    fluent_atoms: list[FluentGroundAtom | str]
    derived_atoms: list[DerivedGroundAtom | str]
    fluent_fterm_values: list[list[FluentGroundFunctionTerm | str | float]]
