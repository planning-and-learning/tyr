from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.ground_atom_view import FluentGroundAtom


class FluentFDRVariable(TypedDict):
    atoms: list[FluentGroundAtom | str]
