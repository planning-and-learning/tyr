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
#include "tyr/datalog/policies/aggregation.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/formalism/arithmetic_operator_utils.hpp"
#include "tyr/formalism/boolean_operator_utils.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <numeric>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

template<TaskKind Kind>
class NumericSupportSelectorWorkspace
{
public:
    struct SelectionEntry
    {
        NumericSupportKeyT<Kind> key;
        ygg::ClosedInterval<ygg::float_t> interval;
        const NumericIntervalAnnotation<Kind>* annotation;
        Cost cost;

        bool operator<(const SelectionEntry& other) const noexcept { return cost < other.cost; }
    };

    void clear() noexcept { selection.clear(); }

    std::vector<SelectionEntry> selection;
};

/// Interval evaluation of ground datalog expressions where each referenced fluent function term is
/// lazily resolved (and recorded) through the selector's support selection.
namespace numeric_support_detail
{
namespace fd = ::tyr::formalism::datalog;
namespace f = ::tyr::formalism;

template<typename Selector>
using Selection = std::vector<typename Selector::SelectionEntry>;

template<typename Selector>
ygg::ClosedInterval<ygg::float_t> evaluate(ygg::float_t value, const Selector&, Selection<Selector>&)
{
    return ygg::ClosedInterval<ygg::float_t>(value, value);
}

template<typename Selector>
ygg::ClosedInterval<ygg::float_t> evaluate(fd::GroundFunctionTermView<f::StaticTag> term, const Selector& selector, Selection<Selector>&)
{
    return selector.lookup_static(term);
}

template<typename Selector>
ygg::ClosedInterval<ygg::float_t> evaluate(fd::GroundFunctionTermView<f::FluentTag> term, const Selector& selector, Selection<Selector>& selection)
{
    return selector.select_fluent_interval(selector.fluent_key(term), selection);
}

template<typename Selector>
ygg::ClosedInterval<ygg::float_t> evaluate(fd::GroundFunctionTermView<f::AuxiliaryTag>, const Selector&, Selection<Selector>&)
{
    return ygg::ClosedInterval<ygg::float_t>();
}

template<typename Selector>
ygg::ClosedInterval<ygg::float_t> evaluate(fd::GroundFunctionExpressionView expression, const Selector& selector, Selection<Selector>& selection);

template<typename Selector>
ygg::ClosedInterval<ygg::float_t> evaluate(fd::GroundArithmeticOperatorView expression, const Selector& selector, Selection<Selector>& selection);

template<typename Selector>
ygg::ClosedInterval<ygg::float_t> evaluate(fd::GroundUnaryOperatorView expression, const Selector& selector, Selection<Selector>& selection)
{
    return f::apply(expression.get_operator(), evaluate(expression.get_arg(), selector, selection));
}

template<typename Selector>
ygg::ClosedInterval<ygg::float_t>
evaluate(fd::GroundBinaryOperatorView<f::ArithmeticOperatorKind> expression, const Selector& selector, Selection<Selector>& selection)
{
    // Sequence the operand evaluations: they append to selection, and function argument evaluation
    // order is unspecified (gcc and clang disagree), which would make the support selection order
    // and hence lmcut tie-breaking compiler-dependent. rhs-first preserves the historical order the
    // fixtures were generated with (and the higher lmcut estimate on the observed instances).
    const auto rhs = evaluate(expression.get_rhs(), selector, selection);
    const auto lhs = evaluate(expression.get_lhs(), selector, selection);
    return f::apply(expression.get_operator(), lhs, rhs);
}

template<typename Selector>
ygg::ClosedInterval<ygg::float_t> evaluate(fd::GroundMultiOperatorView expression, const Selector& selector, Selection<Selector>& selection)
{
    const auto args = expression.get_args();
    if (args.empty())
        return ygg::ClosedInterval<ygg::float_t>();
    return std::accumulate(std::next(args.begin()),
                           args.end(),
                           evaluate(args.front(), selector, selection),
                           [&](const auto& value, const auto& arg) { return f::apply(expression.get_operator(), value, evaluate(arg, selector, selection)); });
}

template<typename Selector>
bool evaluate(fd::GroundBinaryOperatorView<f::BooleanOperatorKind> expression, const Selector& selector, Selection<Selector>& selection)
{
    // Sequenced for the same reason as the arithmetic binary operator above.
    const auto rhs = evaluate(expression.get_rhs(), selector, selection);
    const auto lhs = evaluate(expression.get_lhs(), selector, selection);
    return f::apply_existential(expression.get_operator(), lhs, rhs);
}

template<typename Selector>
ygg::ClosedInterval<ygg::float_t> evaluate(fd::GroundFunctionExpressionView expression, const Selector& selector, Selection<Selector>& selection)
{
    return ygg::visit([&](auto&& arg) { return evaluate(arg, selector, selection); }, expression.get_variant());
}

template<typename Selector>
ygg::ClosedInterval<ygg::float_t> evaluate(fd::GroundArithmeticOperatorView expression, const Selector& selector, Selection<Selector>& selection)
{
    return ygg::visit([&](auto&& arg) { return evaluate(arg, selector, selection); }, expression.get_variant());
}

template<typename Selector>
bool evaluate(fd::GroundBooleanOperatorView expression, const Selector& selector, Selection<Selector>& selection)
{
    return ygg::visit([&](auto&& arg) { return evaluate(arg, selector, selection); }, expression.get_variant());
}

}

/// Shared core of the lifted and ground numeric support selectors: selecting fluent interval supports
/// and pricing numeric constraints against the cost-sorted per-fluent interval annotations.
///
/// Derived provides the tag-specific storage accessors:
///   Key            — fluent function key (function binding view / ground function term view)
///   SelectionEntry — aggregate { key, interval, annotation pointer, cost } ordered by cost
///   key_of(entry), fluent_key(term), lookup_static(term), current_interval(key), find_entries(key),
///   keys_equal(a, b), missing_entries_cost()
///
/// Key and SelectionEntry are template parameters (rather than member typedefs of Derived) because
/// Derived is incomplete while this base is instantiated.
template<typename Derived, typename Key, typename SelectionEntry>
class NumericSupportSelectorCore
{
public:
    using Selection = std::vector<SelectionEntry>;

    ygg::ClosedInterval<ygg::float_t> evaluate_effect_expression(::tyr::formalism::datalog::GroundFunctionExpressionView expression, Selection& selection) const
    {
        return numeric_support_detail::evaluate(expression, derived(), selection);
    }

    bool is_supported(::tyr::formalism::datalog::GroundBooleanOperatorView constraint, Selection& selection) const
    {
        return numeric_support_detail::evaluate(constraint, derived(), selection);
    }

    ygg::ClosedInterval<ygg::float_t> select_fluent_interval(Key key, Selection& selection) const
    {
        if (const auto* entry = find_selection_entry(key, selection))
            return entry->interval;

        const auto current = derived().current_interval(key);
        if (empty(current))
            return ygg::ClosedInterval<ygg::float_t>();

        const auto cost = get_current_interval_cost(key, current);
        if (cost == std::numeric_limits<Cost>::max())
            return ygg::ClosedInterval<ygg::float_t>();

        selection.push_back(SelectionEntry { key, current, nullptr, cost });
        return current;
    }

    template<typename AggregationFunction>
    Cost get_constraint_cost(::tyr::formalism::datalog::GroundBooleanOperatorView constraint, Selection& selection, AggregationFunction agg) const
    {
        return get_greedy_support_cost(selection, agg, [&](auto& selected) { return is_supported(constraint, selected); });
    }

    /// Report a sufficient exact-certificate proof for a selected support entry: its own annotation,
    /// an exact certificate for the selected interval, or the two certificates attaining its bounds.
    /// Returns whether a complete proof was reported.
    template<typename Callback>
    bool for_each_entry_support(const SelectionEntry& entry, Callback callback) const
    {
        const auto key = Derived::key_of(entry);

        if (entry.annotation)
        {
            callback(key, entry.interval, entry.annotation->annotation);
            return true;
        }

        const auto* entries = derived().find_entries(key);
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
    Cost for_each_constraint_support(::tyr::formalism::datalog::GroundBooleanOperatorView constraint,
                                     Selection& selection,
                                     AggregationFunction agg,
                                     Callback callback) const
    {
        const auto cost = get_constraint_cost(constraint, selection, agg);
        if (cost == std::numeric_limits<Cost>::max())
            return cost;

        for (const auto& entry : selection)
            for_each_entry_support(entry, callback);

        return cost;
    }

    /// The cheapest annotation cost at which the achieved interval hull covers `current`.
    Cost get_current_interval_cost(Key key, ygg::ClosedInterval<ygg::float_t> current) const
    {
        const auto* entries = derived().find_entries(key);
        if (!entries)
            return derived().missing_entries_cost();

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
        const auto current = derived().current_interval(key);
        return !empty(current) && subset(interval, current);
    }

private:
    const Derived& derived() const noexcept { return static_cast<const Derived&>(*this); }

    const SelectionEntry* find_selection_entry(Key key, const Selection& selection) const
    {
        for (const auto& entry : selection)
            if (derived().keys_equal(Derived::key_of(entry), key))
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
            const auto key = Derived::key_of(selection[pos]);
            const auto* entries = derived().find_entries(key);
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
};

template<TaskKind Kind>
class NumericSupportSelector :
    public NumericSupportSelectorCore<NumericSupportSelector<Kind>, NumericSupportKeyT<Kind>, typename NumericSupportSelectorWorkspace<Kind>::SelectionEntry>
{
public:
    using Key = NumericSupportKeyT<Kind>;
    using SelectionEntry = typename NumericSupportSelectorWorkspace<Kind>::SelectionEntry;
    using Core = NumericSupportSelectorCore<NumericSupportSelector<Kind>, Key, SelectionEntry>;

    NumericSupportSelector(const FactSets& fact_sets, const NumericIntervalAnnotations<Kind>& annotations, bool initial_intervals_cost_zero = false);

    static Key key_of(const SelectionEntry& entry) noexcept { return entry.key; }
    Key fluent_key(::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag> term) const noexcept;
    ygg::ClosedInterval<ygg::float_t> lookup_static(::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::StaticTag> term) const;
    ygg::ClosedInterval<ygg::float_t> current_interval(Key key) const;
    const typename NumericIntervalAnnotations<Kind>::Entries* find_entries(Key key) const;
    bool keys_equal(Key lhs, Key rhs) const noexcept { return lhs == rhs; }
    Cost missing_entries_cost() const noexcept { return m_initial_intervals_cost_zero ? Cost(0) : std::numeric_limits<Cost>::max(); }

private:
    FactSets m_fact_sets;
    const NumericIntervalAnnotations<Kind>& m_annotations;
    bool m_initial_intervals_cost_zero;
};

using GroundNumericSupportSelector = NumericSupportSelector<GroundTag>;

extern template class NumericSupportSelector<GroundTag>;
extern template class NumericSupportSelector<LiftedTag>;

}

#endif
