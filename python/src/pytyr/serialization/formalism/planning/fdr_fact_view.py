from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import fdr_variable_view




class FluentFDRFact(TypedDict):
    variable: fdr_variable_view.FluentFDRVariable | str
    value: int
