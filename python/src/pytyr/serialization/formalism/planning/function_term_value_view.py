from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_term_view




class StaticGroundFunctionTermValue(TypedDict):
    fterm: function_term_view.StaticGroundFunctionTerm | str
    value: float


class FluentGroundFunctionTermValue(TypedDict):
    fterm: function_term_view.FluentGroundFunctionTerm | str
    value: float


class AuxiliaryGroundFunctionTermValue(TypedDict):
    fterm: function_term_view.AuxiliaryGroundFunctionTerm | str
    value: float
