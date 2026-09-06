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

#ifndef TYR_DATALOG_POLICIES_NUMERIC_SUPPORT_HPP_
#define TYR_DATALOG_POLICIES_NUMERIC_SUPPORT_HPP_

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/numeric_utils.hpp"
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <limits>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

class NumericSupportSelectorWorkspace
{
public:
    struct SelectionEntry
    {
        formalism::datalog::FunctionBindingView<formalism::FluentTag> key;
        ygg::ClosedInterval<ygg::float_t> interval;
        const NumericIntervalAnnotation* annotation;
        Cost cost;

        bool operator<(const SelectionEntry& other) const noexcept { return cost < other.cost; }
    };

    void clear() noexcept { selection.clear(); }

    std::vector<SelectionEntry> selection;
};

/// Selects fluent interval supports and prices numeric constraints against the
/// cost-sorted per-fluent interval annotations.
class NumericSupportSelector
{
public:
    using Key = formalism::datalog::FunctionBindingView<formalism::FluentTag>;
    using SelectionEntry = NumericSupportSelectorWorkspace::SelectionEntry;
    using Selection = std::vector<SelectionEntry>;

    NumericSupportSelector(const FactSets& fact_sets, const NumericIntervalAnnotations<>& annotations, bool initial_intervals_cost_zero = false);

    Key fluent_key(formalism::datalog::FunctionTermView<GroundTag, formalism::FluentTag> term) const noexcept;
    ygg::ClosedInterval<ygg::float_t> lookup_static(formalism::datalog::FunctionTermView<GroundTag, formalism::StaticTag> term) const;
    ygg::ClosedInterval<ygg::float_t> current_interval(Key key) const;
    const NumericIntervalAnnotations<>::Entries* find_entries(Key key) const;
    Cost missing_entries_cost() const noexcept { return m_initial_intervals_cost_zero ? Cost(0) : std::numeric_limits<Cost>::max(); }

    ygg::ClosedInterval<ygg::float_t> evaluate_effect_expression(formalism::datalog::FunctionExpressionView<GroundTag> expression, Selection& selection) const
    {
        return evaluate_numeric_expression(expression, [&](const auto term) { return evaluate_term(term, selection); });
    }

    bool is_supported(formalism::datalog::BooleanOperatorView<GroundTag> constraint, Selection& selection) const
    {
        return evaluate_numeric_expression(constraint, [&](const auto term) { return evaluate_term(term, selection); });
    }

    ygg::ClosedInterval<ygg::float_t> select_fluent_interval(Key key, Selection& selection) const
    {
        if (const auto* entry = find_selection_entry(key, selection))
            return entry->interval;

        const auto current = current_interval(key);
        if (empty(current))
            return ygg::ClosedInterval<ygg::float_t>();

        const auto cost = get_current_interval_cost(key, current);
        if (cost == std::numeric_limits<Cost>::max())
            return ygg::ClosedInterval<ygg::float_t>();

        selection.push_back(SelectionEntry { key, current, nullptr, cost });
        return current;
    }

    template<typename AggregationFunction>
    Cost get_constraint_cost(formalism::datalog::BooleanOperatorView<GroundTag> constraint, Selection& selection, AggregationFunction agg) const
    {
        return get_greedy_support_cost(selection, agg, [&](auto& selected) { return is_supported(constraint, selected); });
    }

    /// Report a sufficient exact-certificate proof for a selected support entry: its own annotation,
    /// an exact certificate for the selected interval, or the two certificates attaining its bounds.
    /// Returns whether a complete proof was reported.
    template<typename Callback>
    bool for_each_entry_support(const SelectionEntry& entry, Callback callback) const
    {
        const auto key = entry.key;

        if (entry.annotation)
        {
            assert(get_cost(entry.annotation->annotation) == entry.cost);
            callback(key, entry.interval, entry.annotation->annotation);
            return true;
        }

        const auto* entries = find_entries(key);
        if (!entries)
            return false;

        auto lower_support = decltype(entry.annotation) {};
        auto upper_support = decltype(entry.annotation) {};
        for (const auto& candidate : *entries)
        {
            const auto cost = get_cost(candidate.annotation);
            if (cost > entry.cost)
                break;
            if (!is_available(key, candidate.interval) || !subset(candidate.interval, entry.interval))
                continue;

            if (candidate.interval == entry.interval)
            {
                assert(cost == entry.cost);
                callback(key, candidate.interval, candidate.annotation);
                return true;
            }

            if (!lower_support && lower(candidate.interval) == lower(entry.interval))
                lower_support = &candidate;
            if (!upper_support && upper(candidate.interval) == upper(entry.interval))
                upper_support = &candidate;
        }

        if (!lower_support || !upper_support)
            return false;

        assert(hull(lower_support->interval, upper_support->interval) == entry.interval);
        assert(std::max(get_cost(lower_support->annotation), get_cost(upper_support->annotation)) == entry.cost);
        callback(key, lower_support->interval, lower_support->annotation);
        if (upper_support != lower_support)
            callback(key, upper_support->interval, upper_support->annotation);
        return true;
    }

    template<typename AggregationFunction, typename Callback>
    Cost for_each_constraint_support(formalism::datalog::BooleanOperatorView<GroundTag> constraint,
                                     Selection& selection,
                                     AggregationFunction agg,
                                     Callback callback) const
    {
        const auto cost = get_constraint_cost(constraint, selection, agg);
        if (cost == std::numeric_limits<Cost>::max())
            return cost;

        for (const auto& entry : selection)
        {
            const auto reported = for_each_entry_support(entry, callback);
            assert(reported);
            static_cast<void>(reported);
        }

        return cost;
    }

    /// The cheapest annotation cost at which the achieved interval hull covers `current`.
    Cost get_current_interval_cost(Key key, ygg::ClosedInterval<ygg::float_t> current) const
    {
        const auto* entries = find_entries(key);
        if (!entries)
            return missing_entries_cost();

        assert(std::is_sorted(entries->begin(),
                              entries->end(),
                              [](const auto& lhs, const auto& rhs) { return get_cost(lhs.annotation) < get_cost(rhs.annotation); }));

        auto best_cost = std::numeric_limits<Cost>::max();
        auto covered = ygg::ClosedInterval<ygg::float_t>();

        for (auto it = entries->begin(); it != entries->end();)
        {
            const auto candidate_cost = get_cost(it->annotation);
            const auto end =
                std::upper_bound(it, entries->end(), candidate_cost, [](Cost cost, const auto& entry) { return cost < get_cost(entry.annotation); });

            for (; it != end; ++it)
                if (is_available(key, it->interval))
                    covered = empty(covered) ? it->interval : hull(covered, it->interval);

            if (!empty(covered) && subset(current, covered))
            {
                best_cost = candidate_cost;
                break;
            }
        }

        return best_cost;
    }

    bool is_available(Key key, ygg::ClosedInterval<ygg::float_t> interval) const
    {
        const auto current = current_interval(key);
        return !empty(current) && subset(interval, current);
    }

private:
    template<formalism::FactKind T>
    ygg::ClosedInterval<ygg::float_t> evaluate_term(formalism::datalog::FunctionTermView<GroundTag, T> term, Selection& selection) const
    {
        if constexpr (std::same_as<T, formalism::StaticTag>)
            return lookup_static(term);
        else if constexpr (std::same_as<T, formalism::FluentTag>)
            return select_fluent_interval(fluent_key(term), selection);
        else
            return {};
    }

    const SelectionEntry* find_selection_entry(Key key, const Selection& selection) const
    {
        for (const auto& entry : selection)
            if (entry.key == key)
                return &entry;
        return nullptr;
    }

    /// Greedily refine the lazily selected fluent supports towards cheaper annotation witnesses,
    /// keeping the constraint supported, then aggregate the selected support costs.
    template<typename AggregationFunction, typename IsSupported>
    Cost get_greedy_support_cost(Selection& selection, AggregationFunction agg, IsSupported is_supported) const
    {
        selection.clear();

        // The first support check evaluates the constraint with current fact intervals.
        // During that evaluation, each referenced fluent function term is lazily added to selection.
        if (!is_supported(selection))
            return std::numeric_limits<Cost>::max();

        // Refine cheaper function supports first. Annotation entries for each key are already sorted by cost.
        // stable_sort: entries are inserted in deterministic evaluation order, so stability makes the
        // tie order (and hence lmcut tie-breaking) identical across standard library implementations
        // (std::sort permutes equal-cost entries differently on libstdc++ vs libc++, which made lmcut
        // values differ between Linux and macOS). Any tie-breaking here stays admissible; measured over
        // the lmcut fixture suite (4 tie orders x 2 acceptance rules), no variant improved more cases
        // than it worsened (interval-width orders changed nothing; reverse order and strictly-cheaper
        // acceptance were net negative), so plain insertion order stays.
        std::stable_sort(selection.begin(), selection.end());

        for (size_t pos = 0; pos < selection.size(); ++pos)
        {
            const auto key = selection[pos].key;
            const auto* entries = find_entries(key);
            if (!entries)
                continue;

            const auto end = std::upper_bound(entries->begin(),
                                              entries->end(),
                                              selection[pos].cost,
                                              [](Cost cost, const auto& entry) { return cost < get_cost(entry.annotation); });

            // Try only witnesses that do not exceed the current support cost. Since entries are cost-sorted,
            // the first candidate that keeps the full constraint supported is the cheapest local replacement.
            for (auto it = entries->begin(); it != end; ++it)
            {
                if (!is_available(key, it->interval))
                    continue;

                const auto old_entry = selection[pos];
                selection[pos] = SelectionEntry { key, it->interval, &*it, get_cost(it->annotation) };
                if (is_supported(selection))
                    break;
                selection[pos] = old_entry;
            }
        }

        auto cost = AggregationFunction::identity();
        for (const auto& entry : selection)
            cost = agg(cost, entry.cost);

        return cost;
    }

    FactSets m_fact_sets;
    const NumericIntervalAnnotations<>& m_annotations;
    bool m_initial_intervals_cost_zero;
};

}

#endif
