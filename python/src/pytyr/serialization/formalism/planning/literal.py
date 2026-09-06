from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import atom


class StaticLiteral(TypedDict):
    atom: atom.StaticAtom | str
    polarity: bool


class FluentLiteral(TypedDict):
    atom: atom.FluentAtom | str
    polarity: bool


class DerivedLiteral(TypedDict):
    atom: atom.DerivedAtom | str
    polarity: bool


class StaticGroundLiteral(TypedDict):
    atom: atom.StaticGroundAtom | str
    polarity: bool


class FluentGroundLiteral(TypedDict):
    atom: atom.FluentGroundAtom | str
    polarity: bool


class DerivedGroundLiteral(TypedDict):
    atom: atom.DerivedGroundAtom | str
    polarity: bool
