from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.binding_view import DerivedPredicateBinding, FluentPredicateBinding, StaticPredicateBinding


class StaticGroundAtom(TypedDict):
    binding: StaticPredicateBinding | str


class FluentGroundAtom(TypedDict):
    binding: FluentPredicateBinding | str


class DerivedGroundAtom(TypedDict):
    binding: DerivedPredicateBinding | str
