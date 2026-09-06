from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.function_view import AuxiliaryFunction, FluentFunction, StaticFunction
from pytyr.serialization.formalism.term_view import Term


class StaticFunctionTerm(TypedDict):
    function: StaticFunction | str
    terms: list[Term | str]


class FluentFunctionTerm(TypedDict):
    function: FluentFunction | str
    terms: list[Term | str]


class AuxiliaryFunctionTerm(TypedDict):
    function: AuxiliaryFunction | str
    terms: list[Term | str]
