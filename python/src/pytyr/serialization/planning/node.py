from __future__ import annotations

from typing import TypedDict
from .state_view import State
from ..formalism.binding_view import ActionBinding





class Node(TypedDict):
    state: State | str
    metric: float


class LabeledNode(TypedDict):
    label: ActionBinding | str
    node: Node
