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

#ifndef TYR_DATALOG_GROUND_WORKSPACES_QUEUE_HPP_
#define TYR_DATALOG_GROUND_WORKSPACES_QUEUE_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/rule_evaluation.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <concepts>
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

struct GroundQueueScratch
{
    using Term = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;

    RuleEvaluationWorkspace rule_evaluation;
    std::vector<Term> changed_terms;

    void clear() noexcept
    {
        rule_evaluation.selector.clear();
        rule_evaluation.exact_supports.clear();
        changed_terms.clear();
    }
};

struct QueueWorkspace
{
    std::vector<GroundQueueEntry<::tyr::formalism::PredicateTag>> predicate_storage;
    std::vector<GroundQueueEntry<::tyr::formalism::FunctionTag>> function_storage;
    GroundQueueStatistics statistics;
    GroundQueueScratch scratch;

    explicit QueueWorkspace(::tyr::formalism::datalog::ProgramView<GroundTag> program)
    {
        predicate_storage.reserve(program.template get_rules<::tyr::formalism::PredicateTag>().size());
        function_storage.reserve(program.template get_rules<::tyr::formalism::FunctionTag>().size());
    }

    template<::tyr::formalism::RelationKind R>
    auto& get_storage() noexcept
    {
        if constexpr (std::same_as<R, ::tyr::formalism::PredicateTag>)
            return predicate_storage;
        else if constexpr (std::same_as<R, ::tyr::formalism::FunctionTag>)
            return function_storage;
        else
            static_assert(ygg::dependent_false<R>::value, "Missing relation kind");
    }

    template<::tyr::formalism::RelationKind R>
    const auto& get_storage() const noexcept
    {
        if constexpr (std::same_as<R, ::tyr::formalism::PredicateTag>)
            return predicate_storage;
        else if constexpr (std::same_as<R, ::tyr::formalism::FunctionTag>)
            return function_storage;
        else
            static_assert(ygg::dependent_false<R>::value, "Missing relation kind");
    }

    void clear()
    {
        predicate_storage.clear();
        function_storage.clear();
        statistics = GroundQueueStatistics {};
        scratch.clear();
    }
};

}

#endif
