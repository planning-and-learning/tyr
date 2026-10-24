# Copyright (C) 2026 Dominik Drexler
# SPDX-License-Identifier: GPL-3.0-or-later
"""Datalog programs and their evaluation."""

# Import all classes for better IDE support

from pytyr._pytyr.datalog import (
    BaseAnnotation as BaseAnnotation,
    FluentFactSets as FluentFactSets,
    FluentFunctionFactSet as FluentFunctionFactSet,
    FluentPredicateFactSet as FluentPredicateFactSet,
    FunctionAnnotations as FunctionAnnotations,
    FunctionWitnessAnnotation as FunctionWitnessAnnotation,
    GroundQueueStatistics as GroundQueueStatistics,
    MaxMinCostAnnotationPolicy as MaxMinCostAnnotationPolicy,
    MaxMinCostAnnotationWithAchieversPolicy as MaxMinCostAnnotationWithAchieversPolicy,
    MaxTerminationPolicy as MaxTerminationPolicy,
    NoAnnotationPolicy as NoAnnotationPolicy,
    NoTerminationPolicy as NoTerminationPolicy,
    NumericSupport as NumericSupport,
    PredicateAnnotations as PredicateAnnotations,
    ProgramStatistics as ProgramStatistics,
    RuleCostPolicy as RuleCostPolicy,
    StaticFactSets as StaticFactSets,
    StaticFunctionFactSet as StaticFunctionFactSet,
    StaticPredicateFactSet as StaticPredicateFactSet,
    SumMinCostAnnotationPolicy as SumMinCostAnnotationPolicy,
    SumTerminationPolicy as SumTerminationPolicy,
    WitnessAnnotation as WitnessAnnotation,
)
from pytyr.datalog import (
    ground as ground,
    lifted as lifted,
)
