from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.object_view import Object


class Term(TypedDict):
    kind: int
    value: Object | str | int
