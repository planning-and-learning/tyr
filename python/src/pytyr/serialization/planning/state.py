from typing import TypedDict
from ..formalism.planning.atom import DerivedGroundAtom, FluentGroundAtom
from ..formalism.planning.function_term import FluentGroundFunctionTerm




class State(TypedDict):
    fluent_ground_atoms: list[FluentGroundAtom | str]
    derived_ground_atoms: list[DerivedGroundAtom | str]
    fluent_ground_function_term_values: list[list[FluentGroundFunctionTerm | str | float]]
