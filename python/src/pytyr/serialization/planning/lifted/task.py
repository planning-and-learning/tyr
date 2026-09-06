from __future__ import annotations

from typing import TypedDict

from ...formalism.planning.planning_task import PlanningTask


class Task(TypedDict):
    formalism_task: PlanningTask
