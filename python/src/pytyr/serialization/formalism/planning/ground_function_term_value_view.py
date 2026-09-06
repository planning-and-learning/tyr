from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.ground_function_term_view import AuxiliaryGroundFunctionTerm, FluentGroundFunctionTerm, StaticGroundFunctionTerm


class StaticGroundFunctionTermValue(TypedDict):
    fterm: StaticGroundFunctionTerm | str
    value: float


class FluentGroundFunctionTermValue(TypedDict):
    fterm: FluentGroundFunctionTerm | str
    value: float


class AuxiliaryGroundFunctionTermValue(TypedDict):
    fterm: AuxiliaryGroundFunctionTerm | str
    value: float
