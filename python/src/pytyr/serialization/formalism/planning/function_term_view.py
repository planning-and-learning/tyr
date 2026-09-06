from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import function_view
from pytyr.serialization.formalism import term_view
from pytyr.serialization.formalism import binding_view


class StaticFunctionTerm(TypedDict):
    function: function_view.StaticFunction | str
    terms: list[term_view.Term | str]


class FluentFunctionTerm(TypedDict):
    function: function_view.FluentFunction | str
    terms: list[term_view.Term | str]


class AuxiliaryFunctionTerm(TypedDict):
    function: function_view.AuxiliaryFunction | str
    terms: list[term_view.Term | str]


class StaticGroundFunctionTerm(TypedDict):
    binding: binding_view.StaticFunctionBinding | str


class FluentGroundFunctionTerm(TypedDict):
    binding: binding_view.FluentFunctionBinding | str


class AuxiliaryGroundFunctionTerm(TypedDict):
    binding: binding_view.AuxiliaryFunctionBinding | str
