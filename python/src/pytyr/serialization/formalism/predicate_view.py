from __future__ import annotations

from typing import TypedDict


class StaticPredicate(TypedDict):
    name: str
    arity: int


class FluentPredicate(TypedDict):
    name: str
    arity: int


class DerivedPredicate(TypedDict):
    name: str
    arity: int
