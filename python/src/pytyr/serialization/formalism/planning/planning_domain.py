from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import domain




class PlanningDomain(TypedDict):
    domain: domain.Domain | str
    path: str | None
