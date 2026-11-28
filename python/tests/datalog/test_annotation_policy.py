# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later

from types import ModuleType

import pytest

from pytyr import datalog


def test_rule_cost_override_policy_remains_task_specific() -> None:
    assert datalog.ground.RuleCostOverridePolicy is not datalog.lifted.RuleCostOverridePolicy


@pytest.mark.parametrize("module", [datalog.ground, datalog.lifted])
def test_annotation_policy_api_is_unified(module: ModuleType) -> None:
    for name in (
        "NoAnnotationPolicy",
        "SumMinCostAnnotationPolicy",
        "MaxMinCostAnnotationPolicy",
    ):
        # The policies are the datalog module's: ground and lifted share them.
        assert getattr(datalog, name)() is not None

    # Its achievers hold rule keys, which differ between the engines.
    assert module.MaxMinCostAnnotationWithAchieversPolicy() is not None
