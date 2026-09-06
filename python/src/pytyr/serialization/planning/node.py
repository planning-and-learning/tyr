from __future__ import annotations

from typing import TypedDict
from .state import State
from ..formalism.binding import ActionBinding





class Node(TypedDict):
    state: State | str
    metric: float


class LabeledNode(TypedDict):
    label: ActionBinding | str
    node: Node | str
