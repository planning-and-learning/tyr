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
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <boost/mp11.hpp>
#include <concepts>
#include <cstdint>
#include <type_traits>
#include <yggdrasil/core/family.hpp>

namespace tyr::datalog
{
/// The fact kinds a rule head derives: each head structure (head updates, annotations, cost buckets, achievers) is held
/// once per kind, a mutable kind more is one more tag here.
using MutableTags = boost::mp11::mp_list<formalism::FluentTag>;

namespace details
{
template<template<typename> class M, typename Tags>
struct MutableFamily;

template<template<typename> class M, typename... Fs>
struct MutableFamily<M, boost::mp11::mp_list<Fs...>>
{
    using type = ygg::Family<M, Fs...>;
};
}

/// One M<F> for each mutable fact kind F (MutableTags), reached by get<F>().
template<template<typename> class M>
using MutableFamily = typename details::MutableFamily<M, MutableTags>::type;

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

class AssignmentSets;

template<formalism::FactKind T>
struct TaggedFactSets;

class FactSets;

enum class SchedulerIterationTrigger : std::uint8_t
{
    AnnotationImproved,
    FactChanged
};

/// What a witness names as the rule instance that derived a fact: a ground rule is its own instance, a lifted one is
/// instantiated by a binding of its variables, its (rule, objects) row.
template<TaskKind Kind>
using RuleKey = std::conditional_t<std::same_as<Kind, GroundTag>, urd::index<formalism::datalog::Rule<GroundTag>>, formalism::datalog::RuleRowIndex>;

class StaticConsistencyGraph;
template<TaskKind Kind>
class Program;
class NoAnnotationPolicy;
template<typename AggregationFunction>
class MinCostAnnotationPolicy;
template<TaskKind Kind, typename AggregationFunction>
class MinCostAnnotationWithAchieversPolicy;
class NoTerminationPolicy;
template<typename AggregationFunction>
class TerminationPolicy;
class RuleCostPolicy;
template<TaskKind Kind>
class RuleCostOverridePolicy;

template<TaskKind Kind>
struct FactsWorkspace;
template<TaskKind Kind>
struct ConstFactsWorkspace;
template<TaskKind Kind, typename AP = NoAnnotationPolicy, typename TP = NoTerminationPolicy, typename CP = RuleCostPolicy>
struct ProgramWorkspace;
template<TaskKind Kind>
struct ConstProgramWorkspace;
template<TaskKind Kind>
struct RuleWorkspace;
template<TaskKind Kind>
struct ConstRuleWorkspace;
template<TaskKind Kind, typename AP = NoAnnotationPolicy, typename TP = NoTerminationPolicy, typename CP = RuleCostPolicy>
struct ProgramExecutionContext;
template<TaskKind Kind>
class Scheduler;
template<>
class Scheduler<GroundTag>;
template<>
class Scheduler<LiftedTag>;

class TypedRuleSchedulerStratum;

struct ProgramStatistics;
struct RuleStatistics;
struct AggregatedRuleStatistics;
struct RuleWorkerStatistics;
struct AggregatedRuleWorkerStatistics;

namespace details
{
template<formalism::FactKind T>
struct Literals;
}

struct VertexAssignment;
struct EdgeAssignment;

}

#endif
