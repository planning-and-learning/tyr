from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import atom_view


class StaticLiteral(TypedDict):
    atom: atom_view.StaticAtom | str
    polarity: bool


class FluentLiteral(TypedDict):
    atom: atom_view.FluentAtom | str
    polarity: bool


class DerivedLiteral(TypedDict):
    atom: atom_view.DerivedAtom | str
    polarity: bool


class StaticGroundLiteral(TypedDict):
    atom: atom_view.StaticGroundAtom | str
    polarity: bool


class FluentGroundLiteral(TypedDict):
    atom: atom_view.FluentGroundAtom | str
    polarity: bool


class DerivedGroundLiteral(TypedDict):
    atom: atom_view.DerivedGroundAtom | str
    polarity: bool
