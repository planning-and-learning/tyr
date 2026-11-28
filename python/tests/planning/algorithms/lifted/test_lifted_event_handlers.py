# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from typing_extensions import override
from pytyr import planning
from python.tests.planning.support.lifted import Node


Plan = planning.lifted.deterministic.PlanBuilderView


IwStatistics = planning.algorithms.iw.lifted.Statistics


SearchStatus = planning.SearchStatus


def test_algorithm_event_handler_subclasses_can_call_super_constructor() -> None:
    class AStarEventHandler(planning.algorithms.astar_eager.lifted.EventHandler):
        def __init__(self) -> None:
            super().__init__()

        @override
        def on_start_search(self, node: Node, f_value: float) -> None:
            pass

        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics) -> None:
            pass

        @override
        def on_solved(self, plan: Plan) -> None:
            pass

    class BRFSEventHandler(planning.algorithms.brfs.lifted.EventHandler):
        def __init__(self) -> None:
            super().__init__()

        @override
        def on_start_search(self, node: Node) -> None:
            pass

        @override
        def on_finish_layer(self, layer: int, statistics: planning.Statistics) -> None:
            pass

        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics) -> None:
            pass

        @override
        def on_solved(self, plan: Plan) -> None:
            pass

    class GBFSEventHandler(planning.algorithms.gbfs_lazy.lifted.EventHandler):
        def __init__(self) -> None:
            super().__init__()

        @override
        def on_start_search(self, node: Node, h_value: float) -> None:
            pass

        @override
        def on_new_best_h_value(self, h_value: float) -> None:
            pass

        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics) -> None:
            pass

        @override
        def on_solved(self, plan: Plan) -> None:
            pass

    class IWEventHandler(planning.algorithms.iw.lifted.EventHandler):
        def __init__(self) -> None:
            super().__init__()
            self.algorithm_statistics = planning.algorithms.iw.lifted.Statistics()

        @override
        def on_start_search(self, max_arity: int) -> None:
            pass

        @override
        def on_start_arity(self, arity: int) -> None:
            pass

        @override
        def on_end_arity(self, arity: int, status: SearchStatus) -> None:
            pass

        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics) -> None:
            pass

        @override
        def on_solved(self, arity: int) -> None:
            pass

        @override
        def get_statistics(self) -> planning.algorithms.iw.lifted.Statistics:
            return self.algorithm_statistics

    class SIWEventHandler(planning.algorithms.siw.lifted.EventHandler):
        def __init__(self) -> None:
            super().__init__()
            self.algorithm_statistics = planning.algorithms.siw.lifted.Statistics()

        @override
        def on_start_search(self) -> None:
            pass

        @override
        def on_start_subsearch(self, subsearch_index: int) -> None:
            pass

        @override
        def add_subsearch_statistics(
            self, search_statistics: planning.Statistics, solver_statistics: IwStatistics
        ) -> None:
            pass

        @override
        def on_end_subsearch(self, subsearch_index: int, status: SearchStatus) -> None:
            pass

        @override
        def on_end_search(self, status: SearchStatus, statistics: planning.Statistics) -> None:
            pass

        @override
        def on_solved(self, plan: Plan) -> None:
            pass

        @override
        def get_statistics(self) -> planning.algorithms.siw.lifted.Statistics:
            return self.algorithm_statistics

    for event_handler_class, event_handler_base in (
        (AStarEventHandler, planning.algorithms.astar_eager.lifted.EventHandler),
        (BRFSEventHandler, planning.algorithms.brfs.lifted.EventHandler),
        (GBFSEventHandler, planning.algorithms.gbfs_lazy.lifted.EventHandler),
        (IWEventHandler, planning.algorithms.iw.lifted.EventHandler),
        (SIWEventHandler, planning.algorithms.siw.lifted.EventHandler),
    ):
        assert isinstance(event_handler_class(), event_handler_base)
