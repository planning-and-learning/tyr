from __future__ import annotations

from typing import TypedDict
from ...formalism.planning.planning_fdr_task import PlanningFDRTask




class Task(TypedDict):
    formalism_task: PlanningFDRTask
