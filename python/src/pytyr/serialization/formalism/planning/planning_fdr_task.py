from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning import fdr_task_view
from pytyr.serialization.formalism.planning import planning_domain




class PlanningFDRTask(TypedDict):
    task: fdr_task_view.GroundTask | str
    domain: planning_domain.PlanningDomain
    path: str | None
