# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later
"""The general environment on ground tasks.

An adversary picks a distribution, chance draws an outcome from it (oneof and probabilistic effects, in any nesting).
"""

# Import all classes for better IDE support

from pytyr._pytyr.planning.ground.general import (
    DistributionBuilder as DistributionBuilder,
    DistributionBuilderView as DistributionBuilderView,
    DistributionData as DistributionData,
    DistributionRecordView as DistributionRecordView,
    OutcomeNodeBuilder as OutcomeNodeBuilder,
    OutcomeNodeBuilderView as OutcomeNodeBuilderView,
    OutcomeNodeData as OutcomeNodeData,
    OutcomeNodeRecordView as OutcomeNodeRecordView,
    OwnedSuccessorBuilderView as OwnedSuccessorBuilderView,
    OwnedSuccessorRecordView as OwnedSuccessorRecordView,
    SuccessorBuilder as SuccessorBuilder,
    SuccessorBuilderView as SuccessorBuilderView,
    SuccessorData as SuccessorData,
    SuccessorRecordView as SuccessorRecordView,
    extend_outcomes as extend_outcomes,
    find_outcomes as find_outcomes,
    for_each_successor as for_each_successor,
    generate_end_successor as generate_end_successor,
    generate_successor as generate_successor,
    generate_timed_literal_successor as generate_timed_literal_successor,
    get_successors as get_successors,
    intern_label as intern_label,
    register_outcomes as register_outcomes,
    require as require,
)
