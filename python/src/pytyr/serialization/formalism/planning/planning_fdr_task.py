from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.fdr_task_view import GroundTask
from pytyr.serialization.formalism.planning.planning_domain import PlanningDomain


class PlanningFDRTask(TypedDict):
    task: GroundTask | str
    domain: PlanningDomain
    path: str | None
