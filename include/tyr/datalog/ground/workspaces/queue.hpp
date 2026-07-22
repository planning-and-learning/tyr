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

#include "tyr/datalog/ground/policies/numeric_support.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/workspaces/queue.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <tuple>
#include <vector>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::datalog
{

struct GroundQueueEntry : ygg::comparison::Mixin<GroundQueueEntry>
{
    Cost cost;
    ygg::uint_t sequence;
    ::tyr::formalism::datalog::GroundRuleView rule;

    GroundQueueEntry() = default;
    GroundQueueEntry(Cost cost, ygg::uint_t sequence, ::tyr::formalism::datalog::GroundRuleView rule) :
        cost(cost),
        sequence(sequence),
        rule(rule)
    {
    }

    auto identifying_members() const noexcept { return std::make_tuple(cost, sequence); }
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
    using SelectionEntry = GroundNumericSupportSelectorWorkspace::SelectionEntry;
    using Term = ::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag>;

    std::vector<SelectionEntry> support_selection;
    std::vector<SelectionEntry> auxiliary_selection;
    std::vector<SelectionEntry> metric_selection;
    std::vector<SelectionEntry> evaluation_selection;
    std::vector<NumericSupport<GroundTag>> numeric_supports;
    SelectedPredicateAnnotations<GroundTag> predicate_annotations;
    SelectedFunctionAnnotations<GroundTag> function_annotations;
    std::vector<Term> changed_terms;

    void clear() noexcept
    {
        support_selection.clear();
        auxiliary_selection.clear();
        metric_selection.clear();
        evaluation_selection.clear();
        numeric_supports.clear();
        predicate_annotations.clear();
        function_annotations.clear();
        changed_terms.clear();
    }
};

template<>
struct QueueWorkspace<GroundTag>
{
    std::vector<GroundQueueEntry> storage;
    GroundQueueStatistics statistics;
    ygg::uint_t next_sequence = 0;
    GroundQueueScratch scratch;

    explicit QueueWorkspace(::tyr::formalism::datalog::ProgramView<GroundTag> program) { storage.reserve(program.get_ground_rules().size()); }

    void clear()
    {
        storage.clear();
        statistics = GroundQueueStatistics {};
        next_sequence = 0;
        scratch.clear();
    }
};

}

#endif
