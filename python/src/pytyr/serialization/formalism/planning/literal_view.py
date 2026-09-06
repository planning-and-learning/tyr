from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.atom_view import DerivedAtom, FluentAtom, StaticAtom


class StaticLiteral(TypedDict):
    atom: StaticAtom | str
    polarity: bool


class FluentLiteral(TypedDict):
    atom: FluentAtom | str
    polarity: bool


class DerivedLiteral(TypedDict):
    atom: DerivedAtom | str
    polarity: bool
