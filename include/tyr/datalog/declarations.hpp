/*
 * Copyright (C) 2025-2026 Dominik Drexler
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TYR_DATALOG_DECLARATIONS_HPP_
#define TYR_DATALOG_DECLARATIONS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

namespace tyr::datalog
{
template<formalism::FactKind T>
class PredicateAssignmentSet;
template<formalism::FactKind T>
class PredicateAssignmentSets;
template<formalism::FactKind T>
class FunctionAssignmentSet;
template<formalism::FactKind T>
class FunctionAssignmentSets;

template<formalism::FactKind T>
struct TaggedAssignmentSets;

struct AssignmentSets;

template<formalism::FactKind T>
class PredicateFactSet;
template<formalism::FactKind T>
class FunctionFactSet;

template<formalism::FactKind T>
struct TaggedFactSets;

struct FactSets;

enum class SchedulerIterationTrigger
{
    AnnotationImproved,
    FactChanged
};

class StaticConsistencyGraph;
template<TaskKind Kind>
class Program;
class NoAnnotationPolicy;
template<typename AggregationFunction>
class MinCostAnnotationPolicy;
template<typename AggregationFunction>
class MinCostAnnotationWithAchieversPolicy;
class NoTerminationPolicy;
template<typename AggregationFunction>
class TerminationPolicy;
class RuleCostPolicy;
template<TaskKind Kind>
class RuleCostOverridePolicy;
class NumericSupportSelectorWorkspace;
class NumericSupportSelector;

struct D2PWorkspace;
template<TaskKind Kind>
struct FactsWorkspace;
template<TaskKind Kind>
struct ConstFactsWorkspace;
struct P2DWorkspace;
template<TaskKind Kind, typename AP = NoAnnotationPolicy, typename TP = NoTerminationPolicy, typename CP = RuleCostPolicy>
struct ProgramWorkspace;
template<TaskKind Kind>
struct ConstProgramWorkspace;
template<TaskKind Kind, formalism::RelationKind R>
struct RuleWorkspace;
template<TaskKind Kind, formalism::RelationKind R>
struct ConstRuleWorkspace;
template<TaskKind Kind, typename AP = NoAnnotationPolicy, typename TP = NoTerminationPolicy, typename CP = RuleCostPolicy>
struct ProgramExecutionContext;
template<TaskKind Kind>
class Scheduler;
template<>
class Scheduler<GroundTag>;
template<>
class Scheduler<LiftedTag>;

template<formalism::RelationKind R>
class TypedRuleSchedulerStratum;

struct ProgramStatistics;
struct RuleStatistics;
struct AggregatedRuleStatistics;
struct RuleWorkerStatistics;
struct AggregatedRuleWorkerStatistics;

namespace details
{
class Vertex;
class Edge;

struct RuleToLiteralInfoMappings;
struct RuleToLiteralPositionMappings;
template<formalism::FactKind T>
struct RuleToLiteralInfo;
template<formalism::FactKind T>
struct TaggedRuleToLiteralInfos;
}

struct VertexAssignment;
struct EdgeAssignment;

}

#endif
