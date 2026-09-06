from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import planning_domain
from pytyr.serialization.formalism.planning import task




class PlanningTask(TypedDict):
    task: task.LiftedTask | str
    domain: planning_domain.PlanningDomain
    path: str | None
