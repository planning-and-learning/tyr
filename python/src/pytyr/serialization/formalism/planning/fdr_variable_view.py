from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import atom_view




class FluentFDRVariable(TypedDict):
    atoms: list[atom_view.FluentGroundAtom | str]
