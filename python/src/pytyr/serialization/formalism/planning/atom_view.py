from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import predicate_view
from pytyr.serialization.formalism import term_view
from pytyr.serialization.formalism import binding_view


class StaticAtom(TypedDict):
    predicate: predicate_view.StaticPredicate | str
    terms: list[term_view.Term | str]


class FluentAtom(TypedDict):
    predicate: predicate_view.FluentPredicate | str
    terms: list[term_view.Term | str]


class DerivedAtom(TypedDict):
    predicate: predicate_view.DerivedPredicate | str
    terms: list[term_view.Term | str]


class StaticGroundAtom(TypedDict):
    binding: binding_view.StaticPredicateBinding | str


class FluentGroundAtom(TypedDict):
    binding: binding_view.FluentPredicateBinding | str


class DerivedGroundAtom(TypedDict):
    binding: binding_view.DerivedPredicateBinding | str
