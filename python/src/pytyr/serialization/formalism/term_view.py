from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism import object_view




class Term(TypedDict):
    kind: str
    value: object_view.Object | str | int
