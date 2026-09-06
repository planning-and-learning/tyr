from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import function
from pytyr.serialization.formalism import term
from pytyr.serialization.formalism import binding


class StaticFunctionTerm(TypedDict):
    function: function.StaticFunction | str
    terms: list[term.Term | str]


class FluentFunctionTerm(TypedDict):
    function: function.FluentFunction | str
    terms: list[term.Term | str]


class AuxiliaryFunctionTerm(TypedDict):
    function: function.AuxiliaryFunction | str
    terms: list[term.Term | str]


class StaticGroundFunctionTerm(TypedDict):
    binding: binding.StaticFunctionBinding | str


class FluentGroundFunctionTerm(TypedDict):
    binding: binding.FluentFunctionBinding | str


class AuxiliaryGroundFunctionTerm(TypedDict):
    binding: binding.AuxiliaryFunctionBinding | str
