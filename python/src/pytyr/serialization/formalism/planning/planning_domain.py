from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import domain_view




class PlanningDomain(TypedDict):
    domain: domain_view.Domain | str
    path: str | None
