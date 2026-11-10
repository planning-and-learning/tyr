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
        "MaxMinCostAnnotationWithAchieversPolicy",
    ):
        # The policies are the datalog module's: ground and lifted share them.
        assert getattr(datalog, name)() is not None

    for name in (
        "NoOrAnnotationPolicy",
        "NoAndAnnotationPolicy",
        "OrAnnotationPolicy",
        "SumAndAnnotationPolicy",
        "MaxAndAnnotationPolicy",
        "MaxAchieverAndAnnotationPolicy",
    ):
        assert not hasattr(module, name)

    for workspace in (module.UnannotatedProgramWorkspace, module.SumProgramWorkspace, module.MaxProgramWorkspace):
        assert hasattr(workspace, "get_annotation_policy")
        assert hasattr(workspace, "get_annotations")
        assert hasattr(workspace, "get_numeric_annotations")


@pytest.mark.parametrize("module", [datalog.ground, datalog.lifted])
def test_annotation_configuration_api_is_unified(module: ModuleType) -> None:
    assert hasattr(module, "MaxAchieverGoalProgramWorkspace")
    assert hasattr(module, "MaxAchieverGoalProgramExecutionContext")
    assert not hasattr(module, "UnannotatedOverrideProgramWorkspace")
    assert not hasattr(module, "UnannotatedOverrideProgramExecutionContext")
