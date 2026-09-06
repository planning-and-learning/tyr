from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import predicate
from pytyr.serialization.formalism import term
from pytyr.serialization.formalism import binding


class StaticAtom(TypedDict):
    predicate: predicate.StaticPredicate | str
    terms: list[term.Term | str]


class FluentAtom(TypedDict):
    predicate: predicate.FluentPredicate | str
    terms: list[term.Term | str]


class DerivedAtom(TypedDict):
    predicate: predicate.DerivedPredicate | str
    terms: list[term.Term | str]


class StaticGroundAtom(TypedDict):
    binding: binding.StaticPredicateBinding | str


class FluentGroundAtom(TypedDict):
    binding: binding.FluentPredicateBinding | str


class DerivedGroundAtom(TypedDict):
    binding: binding.DerivedPredicateBinding | str
