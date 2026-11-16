# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later
"""Evaluation of lifted Datalog programs."""

# Import all classes for better IDE support

from pytyr._pytyr.datalog.lifted import (
    ConstProgramWorkspace as ConstProgramWorkspace,
    MaxAchieverGoalOverrideProgramExecutionContext as MaxAchieverGoalOverrideProgramExecutionContext,
    MaxAchieverGoalOverrideProgramWorkspace as MaxAchieverGoalOverrideProgramWorkspace,
    MaxAchieverGoalProgramExecutionContext as MaxAchieverGoalProgramExecutionContext,
    MaxAchieverGoalProgramWorkspace as MaxAchieverGoalProgramWorkspace,
    MaxGoalOverrideProgramExecutionContext as MaxGoalOverrideProgramExecutionContext,
    MaxGoalOverrideProgramWorkspace as MaxGoalOverrideProgramWorkspace,
    MaxGoalProgramExecutionContext as MaxGoalProgramExecutionContext,
    MaxGoalProgramWorkspace as MaxGoalProgramWorkspace,
    MaxMinCostAnnotationWithAchieversPolicy as MaxMinCostAnnotationWithAchieversPolicy,
    MaxOverrideProgramExecutionContext as MaxOverrideProgramExecutionContext,
    MaxOverrideProgramWorkspace as MaxOverrideProgramWorkspace,
    MaxProgramExecutionContext as MaxProgramExecutionContext,
    MaxProgramWorkspace as MaxProgramWorkspace,
    PredicateAnnotations as PredicateAnnotations,
    Program as Program,
    RuleCostOverridePolicy as RuleCostOverridePolicy,
    SumGoalOverrideProgramExecutionContext as SumGoalOverrideProgramExecutionContext,
    SumGoalOverrideProgramWorkspace as SumGoalOverrideProgramWorkspace,
    SumGoalProgramExecutionContext as SumGoalProgramExecutionContext,
    SumGoalProgramWorkspace as SumGoalProgramWorkspace,
    SumOverrideProgramExecutionContext as SumOverrideProgramExecutionContext,
    SumOverrideProgramWorkspace as SumOverrideProgramWorkspace,
    SumProgramExecutionContext as SumProgramExecutionContext,
    SumProgramWorkspace as SumProgramWorkspace,
    UnannotatedProgramExecutionContext as UnannotatedProgramExecutionContext,
    UnannotatedProgramWorkspace as UnannotatedProgramWorkspace,
    WitnessAnnotation as WitnessAnnotation,
    compute_model as compute_model,
)
from pytyr.datalog.lifted import database as database
