from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import function_term




class StaticGroundFunctionTermValue(TypedDict):
    function_term: function_term.StaticGroundFunctionTerm | str
    value: float


class FluentGroundFunctionTermValue(TypedDict):
    function_term: function_term.FluentGroundFunctionTerm | str
    value: float


class AuxiliaryGroundFunctionTermValue(TypedDict):
    function_term: function_term.AuxiliaryGroundFunctionTerm | str
    value: float
