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

#ifndef TYR_DATALOG_LIFTED_SCHEDULER_HPP_
#define TYR_DATALOG_LIFTED_SCHEDULER_HPP_

#include "tyr/analysis/listeners.hpp"
#include "tyr/analysis/stratification.hpp"
#include "tyr/datalog/cost_buckets.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/rule_index.hpp"

#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <cassert>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog
{

template<::tyr::formalism::RelationKind R>
class TypedRuleSchedulerStratum
{
public:
    TypedRuleSchedulerStratum(const analysis::TypedRuleStratum<R>& rules,
                              const analysis::TypedListenerStratum<R>& listeners,
                              const ::tyr::formalism::datalog::Repository& context,
                              size_t num_fluent_predicates,
                              size_t num_fluent_functions);

    void activate_all();

    void on_start_iteration() noexcept;

    void on_generate(ygg::Index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>> predicate);
    void on_generate(ygg::Index<::tyr::formalism::Function<::tyr::formalism::FluentTag>> function);

    void on_finish_iteration();

    const ::tyr::formalism::datalog::Repository& get_context() const noexcept { return m_context; }
    const ygg::IndexList<::tyr::formalism::datalog::Rule<R>>& get_rules() const noexcept { return m_rules; }

    /// Active rules in sorted index order: hash-set iteration order is platform-unspecified, but the
    /// rule processing order assigns program-repository rows (first-derivation order) that delta
    /// bitsets are indexed by, so it must be identical on every platform.
    const ygg::IndexList<::tyr::formalism::datalog::Rule<R>>& get_active_rules() const noexcept { return m_sorted_active_rules; }

    bool requires_full_enumeration(ygg::Index<::tyr::formalism::datalog::Rule<R>> rule) const noexcept { return m_full_enumeration_rules.contains(rule); }

private:
    void rebuild_sorted_active_rules();

    const analysis::TypedRuleStratum<R>& m_rules;
    const analysis::TypedListenerStratum<R>& m_listeners;
    const ::tyr::formalism::datalog::Repository& m_context;

    boost::dynamic_bitset<> m_active_predicates;
    boost::dynamic_bitset<> m_active_functions;
    ygg::UnorderedSet<ygg::Index<::tyr::formalism::datalog::Rule<R>>> m_active_rules;
    ygg::UnorderedSet<ygg::Index<::tyr::formalism::datalog::Rule<R>>> m_full_enumeration_rules;
    ygg::IndexList<::tyr::formalism::datalog::Rule<R>> m_sorted_active_rules;
};

template<>
class Scheduler<LiftedTag>
{
public:
    Scheduler(const analysis::RuleStratum& rules,
              const analysis::ListenerStratum& listeners,
              const ::tyr::formalism::datalog::Repository& context,
              size_t num_fluent_predicates,
              size_t num_fluent_functions);

    auto& get(::tyr::formalism::PredicateTag) noexcept { return predicate_rules; }
    auto& get(::tyr::formalism::FunctionTag) noexcept { return function_rules; }
    const auto& get(::tyr::formalism::PredicateTag) const noexcept { return predicate_rules; }
    const auto& get(::tyr::formalism::FunctionTag) const noexcept { return function_rules; }

    template<::tyr::formalism::RelationKind R>
    auto& get() noexcept
    {
        return get(R {});
    }

    template<::tyr::formalism::RelationKind R>
    const auto& get() const noexcept
    {
        return get(R {});
    }

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void begin_stratum([[maybe_unused]] ProgramExecutionContext<LiftedTag, AP, TP, CP>& ctx)
    {
        activate_all();
    }

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void begin_iteration(ProgramExecutionContext<LiftedTag, AP, TP, CP>& ctx)
    {
        on_start_iteration();
        ctx.out().delta_annotations().clear();
        ctx.out().delta_numeric_annotations().clear();
    }

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void notify_numeric_changed(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding,
                                [[maybe_unused]] ProgramExecutionContext<LiftedTag, AP, TP, CP>& ctx)
    {
        activate(binding.get_index().relation);
    }

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void notify_generated(const CostBuckets::Bucket& bucket, [[maybe_unused]] ProgramExecutionContext<LiftedTag, AP, TP, CP>& ctx)
    {
        for (const auto fact : bucket.predicate)
            activate(fact.get_index().relation);
        for (const auto& entry : bucket.function)
            activate(entry.first.get_index().relation);
    }

    void finish_iteration([[maybe_unused]] SchedulerIterationTrigger trigger)
    {
#ifdef TYR_ENABLE_SEMI_NAIVE
        on_finish_iteration();
#else
        if (trigger == SchedulerIterationTrigger::AnnotationImproved)
            on_finish_iteration();
        else
            activate_all();
#endif
    }

private:
    void activate_all();
    void on_start_iteration() noexcept;
    void activate(ygg::Index<::tyr::formalism::Predicate<::tyr::formalism::FluentTag>> predicate);
    void activate(ygg::Index<::tyr::formalism::Function<::tyr::formalism::FluentTag>> function);
    void on_finish_iteration();

    TypedRuleSchedulerStratum<::tyr::formalism::PredicateTag> predicate_rules;
    TypedRuleSchedulerStratum<::tyr::formalism::FunctionTag> function_rules;
};

std::vector<Scheduler<LiftedTag>> create_schedulers(const analysis::RuleStrata& rules,
                                                    const analysis::ListenerStrata& listeners,
                                                    const ::tyr::formalism::datalog::Repository& context,
                                                    size_t num_fluent_predicates,
                                                    size_t num_fluent_functions);

}

#endif
