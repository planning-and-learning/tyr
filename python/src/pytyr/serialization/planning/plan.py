from typing import TypedDict
from .node import LabeledNode, Node



class Plan(TypedDict):
    start_node: Node
    labeled_succ_nodes: list[LabeledNode]
    length: int
    cost: float
