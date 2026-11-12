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
#include "tyr/datalog/policies/annotation_concepts.hpp"
#include "tyr/datalog/policies/cost_concepts.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"
#include "tyr/formalism/datalog/declarations.hpp"

#include <cassert>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/formatting/cista_formatters.hpp>

namespace tyr::datalog
{

class TypedRuleSchedulerStratum
{
public:
    TypedRuleSchedulerStratum(const analysis::TypedRuleStratum& rules,
                              const analysis::TypedListenerStratum& listeners,
                              const formalism::datalog::Repository& context,
                              size_t num_fluent_predicates,
                              size_t num_fluent_functions);

    void activate_all();

    void on_start_iteration() noexcept;

    void on_generate(urd::index<formalism::Predicate<formalism::FluentTag>> predicate);
    void on_generate(urd::index<formalism::Function<formalism::FluentTag>> function);

    void on_finish_iteration();

    [[nodiscard]] const formalism::datalog::Repository& get_context() const noexcept { return *m_context; }
    [[nodiscard]] const urd::flat_vector<urd::index<formalism::datalog::Rule<LiftedTag>>>& get_rules() const noexcept { return *m_rules; }

    /// Active rules in index order (the dense set iterates in it): the rule processing order assigns
    /// program-repository rows (first-derivation order) that delta bitsets are indexed by, so it must
    /// be identical on every platform.
    [[nodiscard]] const urd::flat_vector<urd::index<formalism::datalog::Rule<LiftedTag>>>& get_active_rules() const noexcept { return m_sorted_active_rules; }

    [[nodiscard]] bool requires_full_enumeration(urd::index<formalism::datalog::Rule<LiftedTag>> rule) const noexcept
    { return m_full_enumeration_rules.contains(rule); }

private:
    void rebuild_sorted_active_rules();

    const analysis::TypedRuleStratum* m_rules;
    const analysis::TypedListenerStratum* m_listeners;
    const formalism::datalog::Repository* m_context;

    urd::Store<urd::set<urd::index<formalism::Predicate<formalism::FluentTag>>>, urd::dense> m_active_predicates;
    urd::Store<urd::set<urd::index<formalism::Function<formalism::FluentTag>>>, urd::dense> m_active_functions;
    urd::Store<urd::set<urd::index<formalism::datalog::Rule<LiftedTag>>>, urd::dense> m_active_rules;
    urd::Store<urd::set<urd::index<formalism::datalog::Rule<LiftedTag>>>, urd::dense> m_full_enumeration_rules;
    urd::flat_vector<urd::index<formalism::datalog::Rule<LiftedTag>>> m_sorted_active_rules;
};

template<>
class Scheduler<LiftedTag>
{
public:
    Scheduler(const analysis::RuleStratum& rules,
              const analysis::ListenerStratum& listeners,
              const formalism::datalog::Repository& context,
              size_t num_fluent_predicates,
              size_t num_fluent_functions);

    auto& rules() noexcept { return predicate_rules; }
    [[nodiscard]] const auto& rules() const noexcept { return predicate_rules; }

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void begin_stratum([[maybe_unused]] ProgramExecutionContext<LiftedTag, AP, TP, CP>& ctx)
    { activate_all(); }

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void begin_iteration(ProgramExecutionContext<LiftedTag, AP, TP, CP>& ctx)
    {
        on_start_iteration();
        ctx.out().delta_annotations().clear();
    }

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void notify_generated(const CostBuckets::Bucket& bucket, [[maybe_unused]] ProgramExecutionContext<LiftedTag, AP, TP, CP>& ctx)
    {
        for (const auto fact : bucket.predicate)
            activate(fact.relation);
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
    void activate(urd::index<formalism::Predicate<formalism::FluentTag>> predicate);
    void on_finish_iteration();

    TypedRuleSchedulerStratum predicate_rules;
};

urd::Store<urd::sequence<Scheduler<LiftedTag>>> create_schedulers(const analysis::RuleStrata& rules,
                                                                  const analysis::ListenerStrata& listeners,
                                                                  const formalism::datalog::Repository& context,
                                                                  size_t num_fluent_predicates,
                                                                  size_t num_fluent_functions);

}

#endif
