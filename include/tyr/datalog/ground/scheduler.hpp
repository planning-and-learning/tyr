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

#ifndef TYR_DATALOG_GROUND_SCHEDULER_HPP_
#define TYR_DATALOG_GROUND_SCHEDULER_HPP_

#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/cost_buckets.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/ground/workspaces/rule.hpp"
#include "tyr/datalog/policies/annotation_concepts.hpp"
#include "tyr/datalog/policies/cost_concepts.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"
#include "tyr/datalog/rule_evaluation.hpp"
#include "tyr/formalism/repository.hpp"

#include <algorithm>
#include <boost/mp11.hpp>
#include <cassert>
#include <concepts>
#include <limits>
#include <optional>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/family.hpp>

namespace tyr::datalog
{

struct GroundQueueStatistics
{
    ygg::uint_t num_rules_fired = 0;
    ygg::uint_t num_facts_derived = 0;
};

/// How a rule's support takes in a committed condition under annotation policy AP: AP's aggregation (max, sum) of its
/// cost; without annotations a rule has no cost, and its support stays where it starts.
template<typename AP>
auto support_aggregation()
{
    if constexpr (AP::stores_annotations)
        return typename AP::Aggregation {};
    else
        return [](Cost support, Cost) { return support; };
}

/// Where a rule's support starts under annotation policy AP: its aggregation's identity.
template<typename AP>
Cost support_start()
{
    if constexpr (AP::stores_annotations)
        return AP::Aggregation::identity();
    else
        return static_cast<Cost>(0);
}

struct GroundSchedulerScratch
{
    PredicateHeadUpdateFamily<GroundTag> predicate_updates;
    StagedPredicateAnnotation<GroundTag> delta_annotations;

    void clear_updates() noexcept
    {
        predicate_updates.clear();
        delta_annotations.clear();
    }

    void clear() noexcept { clear_updates(); }
};

template<>
class Scheduler<GroundTag>
{
public:
    /// What a run starts from is computed once here, by rule: the number of its conditions (fluent and derived literals and
    /// numeric constraints), the rules without any, the rules with numeric constraints.
    explicit Scheduler(formalism::ProgramView<GroundTag> program) : m_program(program)
    {
        const auto rules = program.get_rules();
        const auto num_slots = formalism::rule_index_bound(rules);
        m_states.resize(num_slots);
        m_conditions.resize(num_slots);
        for (const auto rule : rules)
        {
            const auto rule_index = static_cast<ygg::uint_t>(rule.index().value);
            const auto body = rule.get_body();
            const auto numeric_constraints = body.get_numeric_constraints().size();
            auto num_literals = size_t { 0 };
            boost::mp11::mp_for_each<MutableTags>([&]<typename F>(F) { num_literals += body.template get_literals<F>().size(); });
            m_conditions[rule_index] = static_cast<ygg::uint_t>(num_literals + numeric_constraints);
            if (numeric_constraints != 0)
                m_numeric_rules.push_back(rule);
            if (m_conditions[rule_index] == 0)
                m_unconditional_rules.push_back(rule);
        }
    }

    /// Back to no condition holding, in time linear in the rules (a copy of their condition counts, no reads of the
    /// program), each rule's support at `start` (its aggregation's identity); the context then satisfies the conditions
    /// its facts meet (satisfy).
    void reset(Cost start)
    {
        m_statistics = {};
        m_scratch.clear();
        for (ygg::uint_t rule_index = 0; rule_index < m_conditions.size(); ++rule_index)
        {
            m_states[rule_index].unsatisfied_count = m_conditions[rule_index];
            m_states[rule_index].support = start;
        }
        for (const auto rule : m_numeric_rules)
            m_states[static_cast<ygg::uint_t>(rule.index().value)].numeric_constraint_satisfied.clear();
        m_ready.assign(m_unconditional_rules.begin(), m_unconditional_rules.end());
    }

    /// One condition of `rule` holds, of cost `cost`, aggregated into its support by `aggregate`: a fact of its body is
    /// committed (or given before the run). A rule whose conditions all hold is ready: it fires with its support.
    template<typename Aggregate>
    void satisfy(formalism::RuleView<GroundTag> rule, Cost cost, const Aggregate& aggregate)
    {
        auto& state = m_states[static_cast<ygg::uint_t>(rule.index().value)];
        assert(state.unsatisfied_count > 0);
        state.support = aggregate(state.support, cost);
        if (--state.unsatisfied_count == 0)
            m_ready.push_back(rule);
    }

    /// The rules whose conditions all hold and that have not fired yet; the solver fires and empties them.
    auto& ready() noexcept { return m_ready; }

    /// The cost a ready rule fires with: the aggregate of its conditions' costs.
    [[nodiscard]] Cost support(formalism::RuleView<GroundTag> rule) const noexcept { return m_states[static_cast<ygg::uint_t>(rule.index().value)].support; }

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void begin_stratum(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void begin_iteration([[maybe_unused]] ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx) noexcept
    {
    }

    void finish_iteration(SchedulerIterationTrigger) noexcept {}

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void notify_generated(const CostBuckets::Bucket& bucket, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    [[nodiscard]] const RuleStates& get_states() const noexcept { return m_states; }

    GroundQueueStatistics& statistics() noexcept { return m_statistics; }

    [[nodiscard]] const GroundQueueStatistics& statistics() const noexcept { return m_statistics; }

    GroundSchedulerScratch& scratch() noexcept { return m_scratch; }

    [[nodiscard]] const GroundSchedulerScratch& scratch() const noexcept { return m_scratch; }

private:
    template<formalism::FactKind F, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void notify_predicate_generated(formalism::PredicateRowKey<F> fact, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void update_numeric_constraint_satisfaction(formalism::RuleView<GroundTag> rule, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    template<formalism::FactKind F, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void notify_predicate_generated_for(formalism::PredicateRowKey<F> fact, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    using Rules = urd::Store<urd::sequence<formalism::RuleView<GroundTag>>>;

    formalism::ProgramView<GroundTag> m_program;
    RuleStates m_states;
    /// By rule: the number of its conditions, fluent and derived literals and numeric constraints.
    urd::Store<urd::sequence<ygg::uint_t>> m_conditions;
    Rules m_unconditional_rules;
    Rules m_numeric_rules;
    /// The rules whose conditions all hold and that have not fired yet.
    Rules m_ready;
    GroundQueueStatistics m_statistics;
    GroundSchedulerScratch m_scratch;
};

}

#endif
