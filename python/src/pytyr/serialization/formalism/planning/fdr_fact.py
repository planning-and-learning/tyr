from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import fdr_variable




class FluentFDRFact(TypedDict):
    fdr_variable: fdr_variable.FluentFDRVariable | str
    value: int
