from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.ground_atom_view import DerivedGroundAtom, FluentGroundAtom, StaticGroundAtom


class StaticGroundLiteral(TypedDict):
    atom: StaticGroundAtom | str
    polarity: bool


class FluentGroundLiteral(TypedDict):
    atom: FluentGroundAtom | str
    polarity: bool


class DerivedGroundLiteral(TypedDict):
    atom: DerivedGroundAtom | str
    polarity: bool
