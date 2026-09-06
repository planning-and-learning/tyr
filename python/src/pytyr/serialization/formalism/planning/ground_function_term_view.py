from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import binding_view


class StaticGroundFunctionTerm(TypedDict):
    binding: binding_view.StaticFunctionBinding | str


class FluentGroundFunctionTerm(TypedDict):
    binding: binding_view.FluentFunctionBinding | str


class AuxiliaryGroundFunctionTerm(TypedDict):
    binding: binding_view.AuxiliaryFunctionBinding | str
