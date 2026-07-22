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

#ifndef TYR_DATALOG_LIFTED_WORKSPACES_PROGRAM_HPP_
#define TYR_DATALOG_LIFTED_WORKSPACES_PROGRAM_HPP_

#include "tyr/datalog/lifted/policies/cost.hpp"
#include "tyr/datalog/lifted/policies/numeric_support.hpp"
#include "tyr/datalog/lifted/rule_scheduler.hpp"
#include "tyr/datalog/lifted/workspaces/facts.hpp"
#include "tyr/datalog/lifted/workspaces/rule.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"
#include "tyr/datalog/programs/program.hpp"
#include "tyr/datalog/statistics/program.hpp"
#include "tyr/datalog/workspaces/program.hpp"
#include "tyr/formalism/datalog/builder.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/builder.hpp"

#include <algorithm>
#include <chrono>
#include <map>
#include <oneapi/tbb/enumerable_thread_specific.h>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/semantics/comparators.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog
{

class CostBuckets
{
public:
    using PredicateViewType = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;
    using FunctionViewType = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;
    using PredicateBucket = ygg::UnorderedSet<PredicateViewType>;
    using FunctionBucket = ygg::UnorderedMap<FunctionViewType, ygg::ClosedInterval<ygg::float_t>>;
    using FunctionBucketEntry = std::pair<FunctionViewType, ygg::ClosedInterval<ygg::float_t>>;
    using Cost = datalog::Cost;

    struct Bucket
    {
        PredicateBucket predicates;
        FunctionBucket functions;

        void clear()
        {
            predicates.clear();
            functions.clear();
        }

        [[nodiscard]] bool empty() const noexcept { return predicates.empty() && functions.empty(); }
        [[nodiscard]] size_t size() const noexcept { return predicates.size() + functions.size(); }
    };

    CostBuckets() : m_current(Cost(0)), m_total_size(0) {}

    void clear() noexcept
    {
        m_buckets.clear();
        m_total_size = 0;
        m_current = Cost(0);
    }

    [[nodiscard]] Cost current_cost() const noexcept { return m_current; }
    [[nodiscard]] bool empty() const noexcept { return m_total_size == 0; }

    bool insert(Cost c, PredicateViewType a)
    {
        auto& bucket = m_buckets[c];
        const auto [it, inserted] = bucket.predicates.insert(a);
        if (inserted)
            ++m_total_size;
        return inserted;
    }

    bool insert(Cost c, FunctionViewType f, ygg::ClosedInterval<ygg::float_t> interval)
    {
        auto& bucket = m_buckets[c].functions;
        auto [it, inserted] = bucket.emplace(f, interval);

        if (inserted)
        {
            ++m_total_size;
        }
        else
        {
            it->second = hull(it->second, interval);
        }

        return inserted;
    }

    bool erase(Cost c, PredicateViewType a)
    {
        const auto it = m_buckets.find(c);
        if (it == m_buckets.end())
            return false;

        const auto erased = it->second.predicates.erase(a) > 0;
        if (erased)
            --m_total_size;
        if (it->second.empty())
            m_buckets.erase(it);
        return erased;
    }

    void update(const CostUpdate<LiftedTag>& update, PredicateViewType a)
    {
        if (update.old_cost.has_value())
            erase(*update.old_cost, a);
        insert(update.new_cost, a);
    }

    void clear_current()
    {
        const auto it = m_buckets.find(m_current);
        if (it == m_buckets.end())
            return;
        m_total_size -= it->second.size();
        m_buckets.erase(it);
    }

    bool advance_to_next_nonempty()
    {
        if (m_buckets.empty())
            return false;
        m_current = m_buckets.begin()->first;
        return true;
    }

    const PredicateBucket& get_current_bucket() const
    {
        static const PredicateBucket kEmpty {};
        const auto it = m_buckets.find(m_current);
        return it == m_buckets.end() ? kEmpty : it->second.predicates;
    }

    const FunctionBucket& get_current_function_bucket() const
    {
        static const FunctionBucket kEmpty {};
        const auto it = m_buckets.find(m_current);
        return it == m_buckets.end() ? kEmpty : it->second.functions;
    }

    const std::vector<PredicateViewType>& get_current_bucket_sorted()
    {
        const auto& bucket = get_current_bucket();
        m_predicate_bucket_scratch.assign(bucket.begin(), bucket.end());
        std::sort(m_predicate_bucket_scratch.begin(), m_predicate_bucket_scratch.end());
        return m_predicate_bucket_scratch;
    }

    const std::vector<FunctionBucketEntry>& get_current_function_bucket_sorted()
    {
        const auto& bucket = get_current_function_bucket();
        m_function_bucket_scratch.clear();
        m_function_bucket_scratch.reserve(bucket.size());
        for (const auto& [head, interval] : bucket)
            m_function_bucket_scratch.emplace_back(head, interval);
        std::sort(m_function_bucket_scratch.begin(), m_function_bucket_scratch.end());
        return m_function_bucket_scratch;
    }

private:
    std::map<Cost, Bucket> m_buckets;
    Cost m_current = Cost(0);
    size_t m_total_size = 0;
    std::vector<PredicateViewType> m_predicate_bucket_scratch;
    std::vector<FunctionBucketEntry> m_function_bucket_scratch;
};

template<OrAnnotationPolicyConcept<LiftedTag> OrAP,
         AndAnnotationPolicyConcept<LiftedTag> AndAP,
         TerminationPolicyConcept<LiftedTag> TP,
         RuleCostPolicyConcept<LiftedTag> CP>
struct ProgramWorkspace<LiftedTag, OrAP, AndAP, TP, CP>
{
    const ConstProgramWorkspace<LiftedTag>& const_workspace;
    const ::tyr::formalism::datalog::Repository& program_repository;
    ::tyr::formalism::datalog::Repository& workspace_repository;

    FactsWorkspace<LiftedTag> facts;

    OrAP or_ap;
    SelectedPredicateAnnotations<LiftedTag> and_annot;
    SelectedFunctionAnnotations<LiftedTag> numeric_and_annot;
    std::optional<NumericSupportSelector<LiftedTag>> numeric_support_selector;

    TP tp;
    CP cost_policy;

    std::vector<std::unique_ptr<RuleWorkspace<LiftedTag>::Instance<AndAP>>> rules;

    ::tyr::formalism::planning::Builder planning_builder;
    ::tyr::formalism::datalog::Builder datalog_builder;

    ygg::IndexList<::tyr::formalism::Object> binding;

    RuleSchedulerStrata schedulers;

    CostBuckets cost_buckets;

    ProgramStatistics statistics;

    explicit ProgramWorkspace(Program<LiftedTag>& program, OrAP or_ap = OrAP(), AndAP and_ap = AndAP(), TP tp = TP(), CP cost_policy = CP());

    void clear_costs() { cost_policy.clear(); }
};

template<>
struct ConstProgramWorkspace<LiftedTag>
{
    ConstFactsWorkspace<LiftedTag> facts;

    std::vector<std::optional<ConstRuleWorkspace<LiftedTag>>> rules;

    explicit ConstProgramWorkspace(Program<LiftedTag>& program);
};

}

#endif
