from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import atom




class FluentFDRVariable(TypedDict):
    atoms: list[atom.FluentGroundAtom | str]
