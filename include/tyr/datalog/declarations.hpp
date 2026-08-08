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
template<::tyr::formalism::FactKind T>
class PredicateAssignmentSet;
template<::tyr::formalism::FactKind T>
class PredicateAssignmentSets;
template<::tyr::formalism::FactKind T>
class FunctionAssignmentSet;
template<::tyr::formalism::FactKind T>
class FunctionAssignmentSets;

template<::tyr::formalism::FactKind T>
struct TaggedAssignmentSets;

struct AssignmentSets;

template<::tyr::formalism::FactKind T>
class PredicateFactSet;
template<::tyr::formalism::FactKind T>
class FunctionFactSet;

template<::tyr::formalism::FactKind T>
struct TaggedFactSets;

struct FactSets;

class StaticConsistencyGraph;
template<TaskKind Kind>
class Program;
template<TaskKind Kind>
class NoOrAnnotationPolicy;
template<TaskKind Kind>
class NoAndAnnotationPolicy;
template<TaskKind Kind>
class OrAnnotationPolicy;
template<TaskKind Kind, typename AggregationFunction>
class AndAnnotationPolicy;
template<TaskKind Kind, typename AggregationFunction>
class AchieverAndAnnotationPolicy;
template<TaskKind Kind>
class NoTerminationPolicy;
template<TaskKind Kind, typename AggregationFunction>
class TerminationPolicy;
template<TaskKind Kind>
class RuleCostPolicy;
template<TaskKind Kind>
class RuleCostOverridePolicy;
template<TaskKind Kind>
class NumericSupportSelectorWorkspace;
template<TaskKind Kind>
class NumericSupportSelector;

struct D2PWorkspace;
template<TaskKind Kind>
struct FactsWorkspace;
template<TaskKind Kind>
struct ConstFactsWorkspace;
struct P2DWorkspace;
template<TaskKind Kind>
struct ConstProgramWorkspace;
template<TaskKind Kind, ::tyr::formalism::RelationKind R>
struct RuleWorkspace;
template<TaskKind Kind, ::tyr::formalism::RelationKind R>
struct ConstRuleWorkspace;

template<::tyr::formalism::RelationKind R>
class TypedRuleSchedulerStratum;
struct RuleSchedulerStratum;
struct RuleSchedulerStrata;

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
template<::tyr::formalism::FactKind T>
struct RuleToLiteralInfo;
template<::tyr::formalism::FactKind T>
struct TaggedRuleToLiteralInfos;
}

struct VertexAssignment;
struct EdgeAssignment;

}

#endif
