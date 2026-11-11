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

#ifndef TYR_DATALOG_FACT_SETS_HPP_
#define TYR_DATALOG_FACT_SETS_HPP_

#include "tyr/formalism/datalog/repository.hpp"

#include <cassert>
#include <concepts>
#include <iterator>
#include <ranges>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/ordered_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/repr/values/sparse_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::datalog
{

/// The facts of one predicate: the ids of their rows in the predicate's pool (a bitset that clears only the words it
/// set). Read them as row views with ygg::rows_view(predicate, facts, repository).
template<formalism::FactKind T>
using PredicateFacts = urd::Store<urd::set<urd::index<ygg::Row<formalism::Predicate<T>>>>, urd::dense, urd::proportional_clear>;

/// The facts of one function: the interval of each of its rows, in insertion order.
template<formalism::FactKind T>
using FunctionFacts = urd::Store<urd::map<urd::index<ygg::Row<formalism::Function<T>>>, ygg::ClosedInterval<ygg::float_t>>, urd::dense, urd::insertion_ordered>;

/// The facts of kind T: those of each predicate and of each function, at its index, and the repository their rows are
/// read through.
template<formalism::FactKind T>
struct TaggedFactSets
{
    urd::Store<urd::sequence<PredicateFacts<T>>> predicate;
    urd::Store<urd::sequence<FunctionFacts<T>>> function;
    const formalism::datalog::Repository* repository;

    TaggedFactSets(formalism::datalog::PredicateListView<T> predicates,
                   formalism::datalog::FunctionListView<T> functions,
                   const formalism::datalog::Repository& repository_);
    /// Facts of predicates only (no functions).
    TaggedFactSets(formalism::datalog::PredicateListView<T> predicates, const formalism::datalog::Repository& repository_) :
        predicate(predicates.size()),
        function(),
        repository(&repository_)
    {
    }
    /// With the atoms and function values given.
    TaggedFactSets(formalism::datalog::PredicateListView<T> predicates,
                   formalism::datalog::FunctionListView<T> functions,
                   formalism::datalog::AtomListView<GroundTag, T> atoms,
                   formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values,
                   const formalism::datalog::Repository& repository_);
};

/// The facts of the predicate at `index`, as row views.
template<formalism::FactKind T>
[[nodiscard]] auto rows(const TaggedFactSets<T>& facts, ygg::uint_t index)
{ return ygg::rows_view(urd::index<formalism::Predicate<T>>(index), facts.predicate[index], *facts.repository); }

/// The fact of the function at `index` with row `row`, as a row view.
template<formalism::FactKind T>
[[nodiscard]] auto row(const TaggedFactSets<T>& facts, ygg::uint_t index, urd::index<ygg::Row<formalism::Function<T>>> id)
{ return facts.repository->view(formalism::datalog::FunctionRowIndex<T> { urd::index<formalism::Function<T>>(index), id }); }

/// Adds the fact; whether it is new.
template<formalism::FactKind T>
bool insert(TaggedFactSets<T>& facts, formalism::datalog::PredicateRowIndex<T> fact)
{ return facts.predicate[fact.relation.value].insert(fact.row).second; }

template<formalism::FactKind T>
[[nodiscard]] bool contains(const TaggedFactSets<T>& facts, formalism::datalog::PredicateRowIndex<T> fact)
{ return facts.predicate[fact.relation.value].contains(fact.row); }

/// Widens the fact's interval by `interval` (its hull), adding it where it has none; whether the interval changed.
template<formalism::FactKind T>
bool insert(TaggedFactSets<T>& facts, formalism::datalog::FunctionRowIndex<T> fact, const ygg::ClosedInterval<ygg::float_t>& interval)
{
    auto& values = facts.function[fact.relation.value];
    const auto [it, inserted] = values.try_emplace(fact.row, interval);
    if (inserted)
        return true;
    auto& held = values.at(fact.row);
    const auto widened = hull(held, interval);
    if (widened == held)
        return false;
    held = widened;
    return true;
}

/// The fact's interval; empty where it has none.
template<formalism::FactKind T>
[[nodiscard]] ygg::ClosedInterval<ygg::float_t> value(const TaggedFactSets<T>& facts, formalism::datalog::FunctionRowIndex<T> fact)
{
    const auto& values = facts.function[fact.relation.value];
    return values.contains(fact.row) ? values.at(fact.row) : ygg::ClosedInterval<ygg::float_t>();
}

/// Adds the facts of `other`; whether any fact or interval changed.
template<formalism::FactKind T>
bool insert(TaggedFactSets<T>& facts, const TaggedFactSets<T>& other)
{
    auto changed = false;
    for (ygg::uint_t i = 0; i < other.predicate.size(); ++i)
        for (const auto row : other.predicate[i])
            changed |= facts.predicate[i].insert(row).second;
    for (ygg::uint_t i = 0; i < other.function.size(); ++i)
        for (const auto& [row, interval] : other.function[i])
            changed |= insert(facts, formalism::datalog::FunctionRowIndex<T> { urd::index<formalism::Function<T>>(i), row }, interval);
    return changed;
}

/// Removes every fact, keeping storage.
template<formalism::FactKind T>
void clear(TaggedFactSets<T>& facts) noexcept
{
    for (auto& rows : facts.predicate)
        rows.clear();
    for (auto& values : facts.function)
        values.clear();
}

template<formalism::FactKind T>
TaggedFactSets<T>::TaggedFactSets(formalism::datalog::PredicateListView<T> predicates,
                                  formalism::datalog::FunctionListView<T> functions,
                                  const formalism::datalog::Repository& repository_) :
    predicate(predicates.size()),
    function(functions.size()),
    repository(&repository_)
{
    for (ygg::uint_t i = 0; i < predicates.size(); ++i)
        assert(predicates[i].index().value == i);
    for (ygg::uint_t i = 0; i < functions.size(); ++i)
        assert(functions[i].index().value == i);
}

template<formalism::FactKind T>
TaggedFactSets<T>::TaggedFactSets(formalism::datalog::PredicateListView<T> predicates,
                                  formalism::datalog::FunctionListView<T> functions,
                                  formalism::datalog::AtomListView<GroundTag, T> atoms,
                                  formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values,
                                  const formalism::datalog::Repository& repository_) :
    TaggedFactSets(predicates, functions, repository_)
{
    for (const auto atom : atoms)
        insert(*this, atom.get_row().data());
    for (const auto fterm_value : fterm_values)
        insert(*this, fterm_value.get_fterm().get_row().data(), ygg::ClosedInterval<ygg::float_t>(fterm_value.get_value(), fterm_value.get_value()));
}

class FactSets
{
public:
    FactSets(const TaggedFactSets<formalism::StaticTag>& static_sets, const TaggedFactSets<formalism::FluentTag>& fluent_sets) noexcept :
        m_static_sets(&static_sets),
        m_fluent_sets(&fluent_sets)
    {
    }

    template<formalism::FactKind T>
        requires std::same_as<T, formalism::StaticTag> || std::same_as<T, formalism::FluentTag>
    [[nodiscard]] const TaggedFactSets<T>& get() const noexcept
    {
        if constexpr (std::same_as<T, formalism::StaticTag>)
            return *m_static_sets;
        else
            return *m_fluent_sets;
    }

private:
    const TaggedFactSets<formalism::StaticTag>* m_static_sets;
    const TaggedFactSets<formalism::FluentTag>* m_fluent_sets;
};

}

#endif
