from __future__ import annotations

from typing import TypedDict

from pytyr.serialization.formalism.planning.planning_domain import PlanningDomain
from pytyr.serialization.formalism.planning.task_view import LiftedTask


class PlanningTask(TypedDict):
    task: LiftedTask | str
    domain: PlanningDomain
    path: str | None
