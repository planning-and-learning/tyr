from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.domain_view import Domain


class PlanningDomain(TypedDict):
    domain: Domain | str
    path: str | None
