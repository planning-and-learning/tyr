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
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"
#include "tyr/datalog/rule_evaluation.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <algorithm>
#include <concepts>
#include <functional>
#include <limits>
#include <optional>
#include <tuple>
#include <vector>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::datalog
{

template<::tyr::formalism::RelationKind R>
struct GroundQueueEntry : ygg::comparison::Mixin<GroundQueueEntry<R>>
{
    Cost cost;
    ::tyr::formalism::datalog::GroundRuleView<R> rule;

    GroundQueueEntry() = delete;
    GroundQueueEntry(Cost cost, ::tyr::formalism::datalog::GroundRuleView<R> rule) : cost(cost), rule(rule) {}

    auto identifying_members() const noexcept { return std::make_tuple(cost, rule); }
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
    RuleEvaluationWorkspace rule_evaluation;
    PredicateHeadUpdates predicate_updates;
    FunctionHeadUpdates function_updates;
    PredicateAnnotations<true> delta_annotations;
    FunctionAnnotations<true> delta_numeric_annotations;
    std::vector<CostBuckets::PredicateKey> changed_facts;
    std::vector<CostBuckets::FunctionKey> changed_terms;

    explicit GroundSchedulerScratch(::tyr::formalism::datalog::ProgramView<GroundTag> program) :
        delta_annotations(program.template get_predicates<::tyr::formalism::FluentTag>().size()),
        delta_numeric_annotations(program.template get_functions<::tyr::formalism::FluentTag>().size())
    {
    }

    void clear_updates() noexcept
    {
        predicate_updates.clear();
        function_updates.clear();
        delta_annotations.clear();
        delta_numeric_annotations.clear();
    }

    void clear() noexcept
    {
        rule_evaluation.selector.clear();
        rule_evaluation.exact_supports.clear();
        changed_facts.clear();
        changed_terms.clear();
        clear_updates();
    }
};

template<>
class Scheduler<GroundTag>
{
public:
    explicit Scheduler(::tyr::formalism::datalog::ProgramView<GroundTag> program) :
        m_program(program),
        m_predicate_rules(program),
        m_function_rules(program),
        m_scratch(program)
    {
        m_predicate_queue.reserve(program.template get_rules<::tyr::formalism::PredicateTag>().size());
        m_function_queue.reserve(program.template get_rules<::tyr::formalism::FunctionTag>().size());
    }

    void reset(const FactSets& fact_sets)
    {
        m_predicate_queue.clear();
        m_function_queue.clear();
        m_statistics = {};
        m_scratch.clear();
        initialize_rule_states<::tyr::formalism::PredicateTag>(fact_sets);
        initialize_rule_states<::tyr::formalism::FunctionTag>(fact_sets);
    }

    template<::tyr::formalism::RelationKind R>
    void enqueue(::tyr::formalism::datalog::GroundRuleView<R> rule, Cost queue_label)
    {
        auto& state = get_states<R>()[ygg::uint_t(rule.get_index())];
        if (state.unsatisfied_count != 0)
            return;

        if (state.queued_cost && *state.queued_cost <= queue_label)
            return;
        state.queued_cost = queue_label;

        auto& queue = get_queue<R>();
        queue.emplace_back(queue_label, rule);
        std::push_heap(queue.begin(), queue.end(), std::greater<> {});

        ++m_statistics.num_queue_pushes;
        m_statistics.max_queue_size = std::max(m_statistics.max_queue_size, static_cast<ygg::uint_t>(m_predicate_queue.size() + m_function_queue.size()));
    }

    template<::tyr::formalism::RelationKind R>
    std::optional<GroundQueueEntry<R>> pop()
    {
        auto& queue = get_queue<R>();
        if (queue.empty())
            return std::nullopt;

        std::pop_heap(queue.begin(), queue.end(), std::greater<> {});
        auto entry = queue.back();
        queue.pop_back();
        ++m_statistics.num_queue_pops;
        return entry;
    }

    template<::tyr::formalism::RelationKind R>
    bool claim(const GroundQueueEntry<R>& entry)
    {
        auto& queued_cost = get_states<R>()[ygg::uint_t(entry.rule.get_index())].queued_cost;
        if (!queued_cost || *queued_cost != entry.cost)
        {
            ++m_statistics.num_stale_queue_pops;
            return false;
        }
        queued_cost.reset();
        return true;
    }

    template<::tyr::formalism::RelationKind R>
    Cost next_cost() const noexcept
    {
        const auto& queue = get_queue<R>();
        return queue.empty() ? std::numeric_limits<Cost>::max() : queue.front().cost;
    }

    Cost next_cost() const noexcept { return std::min(next_cost<::tyr::formalism::PredicateTag>(), next_cost<::tyr::formalism::FunctionTag>()); }

    template<::tyr::formalism::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void schedule(::tyr::formalism::datalog::GroundRuleView<R> rule, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void seed(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void on_generate(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> fact, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void on_generate(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> term, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void on_generate(const CostBuckets::Bucket& bucket, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    template<::tyr::formalism::RelationKind R>
    auto& get_states() noexcept
    {
        return get_rules(R {}).states;
    }

    template<::tyr::formalism::RelationKind R>
    const auto& get_states() const noexcept
    {
        return get_rules(R {}).states;
    }

    GroundQueueStatistics& statistics() noexcept { return m_statistics; }
    const GroundQueueStatistics& statistics() const noexcept { return m_statistics; }
    GroundSchedulerScratch& scratch() noexcept { return m_scratch; }
    const GroundSchedulerScratch& scratch() const noexcept { return m_scratch; }

private:
    template<::tyr::formalism::RelationKind R>
    void initialize_rule_states(const FactSets& fact_sets)
    {
        const auto rules = m_program.template get_rules<R>();
        auto& states = get_states<R>();
        states.resize(rules.size());
        for (ygg::uint_t rule_index = 0; rule_index < rules.size(); ++rule_index)
        {
            const auto rule = rules[rule_index];
            auto& state = states[rule_index];

            auto unsatisfied_count = ygg::uint_t(0);
            for (const auto literal : rule.get_body().template get_literals<::tyr::formalism::FluentTag>())
                if (!is_applicable(literal, fact_sets))
                    ++unsatisfied_count;

            const auto numeric_constraints = rule.get_body().get_numeric_constraints();
            state.numeric_constraint_satisfied.assign(numeric_constraints.size(), false);
            unsatisfied_count += numeric_constraints.size();
            state.unsatisfied_count = unsatisfied_count;
            state.queued_cost.reset();
        }
    }

    template<::tyr::formalism::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void update_numeric_constraint_satisfaction(::tyr::formalism::datalog::GroundRuleView<R> rule, ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    template<::tyr::formalism::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void on_generate_predicate(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> fact,
                               ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    template<::tyr::formalism::RelationKind R, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
    void on_generate_function(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> term,
                              ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

    auto& get_queue(::tyr::formalism::PredicateTag) noexcept { return m_predicate_queue; }
    auto& get_queue(::tyr::formalism::FunctionTag) noexcept { return m_function_queue; }
    const auto& get_queue(::tyr::formalism::PredicateTag) const noexcept { return m_predicate_queue; }
    const auto& get_queue(::tyr::formalism::FunctionTag) const noexcept { return m_function_queue; }

    template<::tyr::formalism::RelationKind R>
    auto& get_queue() noexcept
    {
        return get_queue(R {});
    }

    template<::tyr::formalism::RelationKind R>
    const auto& get_queue() const noexcept
    {
        return get_queue(R {});
    }

    auto& get_rules(::tyr::formalism::PredicateTag) noexcept { return m_predicate_rules; }
    auto& get_rules(::tyr::formalism::FunctionTag) noexcept { return m_function_rules; }
    const auto& get_rules(::tyr::formalism::PredicateTag) const noexcept { return m_predicate_rules; }
    const auto& get_rules(::tyr::formalism::FunctionTag) const noexcept { return m_function_rules; }

    ::tyr::formalism::datalog::ProgramView<GroundTag> m_program;
    RuleWorkspace<GroundTag, ::tyr::formalism::PredicateTag> m_predicate_rules;
    RuleWorkspace<GroundTag, ::tyr::formalism::FunctionTag> m_function_rules;
    std::vector<GroundQueueEntry<::tyr::formalism::PredicateTag>> m_predicate_queue;
    std::vector<GroundQueueEntry<::tyr::formalism::FunctionTag>> m_function_queue;
    GroundQueueStatistics m_statistics;
    GroundSchedulerScratch m_scratch;
};

}

#endif
