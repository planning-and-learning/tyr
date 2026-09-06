from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import object




class Term(TypedDict):
    kind: str
    value: object.Object | str | int
