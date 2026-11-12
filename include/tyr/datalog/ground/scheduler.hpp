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
#include "tyr/formalism/datalog/repository.hpp"

#include <algorithm>
#include <concepts>
#include <limits>
#include <optional>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/repr/queues/queue.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/family.hpp>

namespace tyr::datalog
{

struct GroundQueueEntry : urd::compared_by_identity<GroundQueueEntry>
{
    Cost cost;
    formalism::datalog::RuleView<GroundTag> rule;

    GroundQueueEntry() = delete;
    GroundQueueEntry(Cost cost_, formalism::datalog::RuleView<GroundTag> rule_) : cost(cost_), rule(rule_) {}

    static constexpr auto identifying_members() noexcept { return urd::Members<&GroundQueueEntry::cost, &GroundQueueEntry::rule> {}; }
};

struct GroundQueueStatistics
{
    ygg::uint_t num_queue_pushes = 0;
    ygg::uint_t num_queue_pops = 0;
    ygg::uint_t num_stale_queue_pops = 0;
    ygg::uint_t num_rules_fired = 0;
    ygg::uint_t num_facts_derived = 0;
    ygg::uint_t max_queue_size = 0;
};

struct GroundSchedulerScratch
{
    PredicateHeadUpdates predicate_updates;
    StagedPredicateAnnotation delta_annotations;
    urd::Store<urd::sequence<formalism::datalog::PredicateRowIndex<formalism::FluentTag>>> head_bindings;
    urd::Store<urd::sequence<CostBuckets::PredicateKey>> changed_facts;

    void clear_updates() noexcept
    {
        predicate_updates.clear();
        delta_annotations.clear();
    }

    void clear() noexcept
    {
        changed_facts.clear();
        clear_updates();
    }
};

template<>
class Scheduler<GroundTag>
{
public:
    explicit Scheduler(formalism::datalog::ProgramView<GroundTag> program) : m_program(program) { m_queue.reserve(program.get_rules().size()); }

    void reset(const FactSets& fact_sets)
    {
        m_queue.clear();
        m_statistics = {};
        m_scratch.clear();
        initialize_rule_states(fact_sets);
    }

    void enqueue(formalism::datalog::RuleView<GroundTag> rule, Cost queue_label)
    {
        auto& state = m_states[static_cast<ygg::uint_t>(rule.index().value)];
        if (state.unsatisfied_count != 0)
            return;

        if (state.queued_cost && *state.queued_cost <= queue_label)
            return;
        state.queued_cost = queue_label;

        m_queue.emplace(queue_label, rule);

        ++m_statistics.num_queue_pushes;
        m_statistics.max_queue_size = std::max(m_statistics.max_queue_size, static_cast<ygg::uint_t>(m_queue.size()));
    }

    std::optional<GroundQueueEntry> pop()
    {
        if (m_queue.empty())
            return std::nullopt;

        auto entry = m_queue.top();
        m_queue.pop();
        ++m_statistics.num_queue_pops;
        return entry;
    }

    bool claim(const GroundQueueEntry& entry)
    {
        auto& queued_cost = m_states[static_cast<ygg::uint_t>(entry.rule.index().value)].queued_cost;
        if (!queued_cost || *queued_cost != entry.cost)
        {
            ++m_statistics.num_stale_queue_pops;
            return false;
        }
        queued_cost.reset();
        return true;
    }

    [[nodiscard]] Cost next_cost() const noexcept { return m_queue.empty() ? std::numeric_limits<Cost>::max() : m_queue.top().cost; }

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
    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void schedule(formalism::datalog::RuleView<GroundTag> rule, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void notify_predicate_generated(formalism::datalog::PredicateRowIndex<formalism::FluentTag> fact, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    void initialize_rule_states(const FactSets& fact_sets)
    {
        const auto rules = m_program.get_rules();
        auto& states = m_states;
        states.resize(rules.size());
        for (ygg::uint_t rule_index = 0; rule_index < rules.size(); ++rule_index)
        {
            const auto rule = rules[rule_index];
            auto& state = states[rule_index];

            auto unsatisfied_count = static_cast<ygg::uint_t>(0);
            for (const auto literal : rule.get_body().template get_literals<formalism::FluentTag>())
                if (!is_applicable(literal, fact_sets))
                    ++unsatisfied_count;

            const auto numeric_constraints = rule.get_body().get_numeric_constraints();
            state.numeric_constraint_satisfied.clear();
            unsatisfied_count += numeric_constraints.size();
            state.unsatisfied_count = unsatisfied_count;
            state.queued_cost.reset();
        }
    }

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void update_numeric_constraint_satisfaction(formalism::datalog::RuleView<GroundTag> rule, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void notify_predicate_generated_for(formalism::datalog::PredicateRowIndex<formalism::FluentTag> fact, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    using Queue = urd::Store<urd::queue<GroundQueueEntry>, urd::by_priority>;

    formalism::datalog::ProgramView<GroundTag> m_program;
    RuleStates m_states;
    Queue m_queue;
    GroundQueueStatistics m_statistics;
    GroundSchedulerScratch m_scratch;
};

}

#endif
