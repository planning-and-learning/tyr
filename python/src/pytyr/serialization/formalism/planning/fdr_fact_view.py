from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.fdr_variable_view import FluentFDRVariable


class FluentFDRFact(TypedDict):
    variable: FluentFDRVariable | str
    value: int
