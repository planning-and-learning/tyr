from __future__ import annotations

from typing import TypedDict



class StaticFunction(TypedDict):
    name: str
    arity: int


class FluentFunction(TypedDict):
    name: str
    arity: int


class AuxiliaryFunction(TypedDict):
    name: str
    arity: int
