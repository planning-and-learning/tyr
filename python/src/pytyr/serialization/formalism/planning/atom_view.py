from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.predicate_view import DerivedPredicate, FluentPredicate, StaticPredicate
from pytyr.serialization.formalism.term_view import Term


class StaticAtom(TypedDict):
    predicate: StaticPredicate | str
    terms: list[Term | str]


class FluentAtom(TypedDict):
    predicate: FluentPredicate | str
    terms: list[Term | str]


class DerivedAtom(TypedDict):
    predicate: DerivedPredicate | str
    terms: list[Term | str]
