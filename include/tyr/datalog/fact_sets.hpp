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

#include <urd/repr/maps/index_map.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <cassert>
#include <iterator>
#include <ranges>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>

namespace tyr::datalog
{

namespace detail
{

/// The rows of a predicate's facts: a bitset over row indices that clears only the words it set.
using PredicateFactRows = urd::Store<urd::set<ygg::uint_t>, urd::dense, urd::proportional_clear>;

/// A fact's row from its bit.
struct RowOfFact
{
    urd::index<formalism::Row> operator()(ygg::uint_t row) const noexcept { return urd::index<formalism::Row> { row }; }
};

using PredicateFactRowRange = std::ranges::transform_view<std::ranges::ref_view<const PredicateFactRows>, RowOfFact>;

template<formalism::FactKind T>
using PredicateBindingRangeView = formalism::RelationBindingsView<formalism::Predicate<T>, PredicateFactRowRange, formalism::datalog::Repository>;

}

template<formalism::FactKind T>
using PredicateBindingViewRange = detail::PredicateBindingRangeView<T>;

template<formalism::FactKind T>
class PredicateFactSet
{
private:
    formalism::datalog::PredicateView<T> m_predicate;
    const formalism::datalog::Repository* m_repository;

    urd::index<formalism::Predicate<T>> m_predicate_index;

    detail::PredicateFactRows m_rows;

public:
    explicit PredicateFactSet(formalism::datalog::PredicateView<T> predicate, const formalism::datalog::Repository& repository);

    [[nodiscard]] auto get_predicate() const noexcept { return m_predicate; }

    void reset() noexcept;

    bool insert(const PredicateFactSet<T>& other);
    bool insert(formalism::datalog::AtomView<GroundTag, T> ground_atom);
    bool insert(formalism::datalog::PredicateBindingIndex<T> binding);
    bool insert(formalism::datalog::PredicateBindingView<T> binding) { return insert(binding.index()); }
    bool insert(formalism::datalog::PredicateBindingForwardRangeView<T> bindings);
    bool insert(const urd::Store<urd::sequence<formalism::datalog::PredicateBindingView<T>>>& bindings);

    [[nodiscard]] bool contains(formalism::datalog::PredicateBindingIndex<T> binding) const noexcept;
    [[nodiscard]] bool contains(formalism::datalog::PredicateBindingView<T> binding) const noexcept { return contains(binding.index()); }

    [[nodiscard]] PredicateBindingViewRange<T> get_bindings() const noexcept;
};

template<formalism::FactKind T>
class PredicateFactSets
{
private:
    urd::Store<urd::sequence<PredicateFactSet<T>>> m_sets;

public:
    explicit PredicateFactSets(formalism::datalog::PredicateListView<T> predicates, const formalism::datalog::Repository& repository);

    void reset() noexcept;

    bool insert(const PredicateFactSets<T>& other);
    bool insert(formalism::datalog::AtomView<GroundTag, T> ground_atom);
    bool insert(formalism::datalog::PredicateBindingIndex<T> binding);
    bool insert(formalism::datalog::PredicateBindingView<T> binding) { return insert(binding.index()); }
    bool insert(formalism::datalog::PredicateBindingForwardRangeView<T> bindings);

    [[nodiscard]] bool contains(formalism::datalog::PredicateBindingIndex<T> binding) const noexcept;
    [[nodiscard]] bool contains(formalism::datalog::PredicateBindingView<T> binding) const noexcept { return contains(binding.index()); }

    [[nodiscard]] const urd::Store<urd::sequence<PredicateFactSet<T>>>& get_sets() const noexcept;
};

template<formalism::FactKind T>
class FunctionFactSet
{
private:
    formalism::datalog::FunctionView<T> m_function;
    const formalism::datalog::Repository* m_repository;

    urd::index<formalism::Function<T>> m_function_index;
    urd::Store<urd::map<urd::index<formalism::Row>, ygg::uint_t>, urd::dense, urd::constant_clear> m_remap;
    urd::Store<urd::sequence<urd::index<formalism::Row>>> m_bindings;
    urd::Store<urd::sequence<ygg::ClosedInterval<ygg::float_t>>> m_values;

public:
    explicit FunctionFactSet(formalism::datalog::FunctionView<T> function, const formalism::datalog::Repository& repository);

    [[nodiscard]] auto get_function() const noexcept { return m_function; }

    void reset() noexcept;

    bool insert(const FunctionFactSet& other);
    bool insert(formalism::datalog::FunctionBindingIndex<T> binding, const ygg::ClosedInterval<ygg::float_t>& interval);
    bool insert(formalism::datalog::FunctionBindingView<T> binding, const ygg::ClosedInterval<ygg::float_t>& interval)
    { return insert(binding.index(), std::move(interval)); }
    bool insert(formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value);
    bool insert(formalism::datalog::FunctionBindingRandomAccessRangeView<T> bindings, const urd::Store<urd::sequence<ygg::ClosedInterval<ygg::float_t>>>& intervals);
    bool insert(formalism::datalog::FunctionBindingRandomAccessRangeView<T> bindings, const urd::Store<urd::sequence<ygg::float_t>>& values);
    bool insert(const urd::Store<urd::sequence<formalism::datalog::FunctionBindingView<T>>>& bindings, const urd::Store<urd::sequence<ygg::ClosedInterval<ygg::float_t>>>& intervals);
    bool insert(const urd::Store<urd::sequence<formalism::datalog::FunctionBindingView<T>>>& bindings, const urd::Store<urd::sequence<ygg::float_t>>& values);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> fterm, const ygg::ClosedInterval<ygg::float_t>& interval);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> fterm, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermValueView<GroundTag, T> fterm_value);
    bool insert(formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values);

    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingIndex<T> binding) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingView<T> binding) const noexcept { return (*this)[binding.index()]; }
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionTermView<GroundTag, T> fterm) const noexcept;

    [[nodiscard]] formalism::datalog::FunctionBindingRandomAccessRangeView<T> get_bindings() const noexcept;
    [[nodiscard]] const urd::Store<urd::sequence<ygg::ClosedInterval<ygg::float_t>>>& get_values() const noexcept;
};

template<formalism::FactKind T>
class FunctionFactSets
{
private:
    urd::Store<urd::sequence<FunctionFactSet<T>>> m_sets;

public:
    explicit FunctionFactSets(formalism::datalog::FunctionListView<T> functions, const formalism::datalog::Repository& repository);

    void reset() noexcept;

    bool insert(const FunctionFactSets& other);
    bool insert(formalism::datalog::FunctionBindingIndex<T> binding, const ygg::ClosedInterval<ygg::float_t>& interval);
    bool insert(formalism::datalog::FunctionBindingView<T> binding, const ygg::ClosedInterval<ygg::float_t>& interval)
    { return insert(binding.index(), interval); }
    bool insert(formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, const ygg::ClosedInterval<ygg::float_t>& interval);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermListView<GroundTag, T> function_terms, const urd::Store<urd::sequence<ygg::float_t>>& values);
    bool insert(formalism::datalog::FunctionTermValueView<GroundTag, T> fterm_value);
    bool insert(formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values);

    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingIndex<T> binding) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingView<T> binding) const noexcept { return (*this)[binding.index()]; }
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionTermView<GroundTag, T> fterm) const noexcept;

    [[nodiscard]] const urd::Store<urd::sequence<FunctionFactSet<T>>>& get_sets() const noexcept;
};

template<formalism::FactKind T>
struct TaggedFactSets
{
    PredicateFactSets<T> predicate;
    FunctionFactSets<T> function;

    TaggedFactSets(formalism::datalog::PredicateListView<T> predicates,
                   formalism::datalog::FunctionListView<T> functions,
                   const formalism::datalog::Repository& repository);

    TaggedFactSets(formalism::datalog::PredicateListView<T> predicates,
                   formalism::datalog::FunctionListView<T> functions,
                   formalism::datalog::AtomListView<GroundTag, T> atoms,
                   formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values,
                   const formalism::datalog::Repository& repository);

    void insert(const TaggedFactSets<T>& other);

    void reset() noexcept;
};

class FactSets
{
public:
    FactSets(const TaggedFactSets<formalism::StaticTag>& static_sets, const TaggedFactSets<formalism::FluentTag>& fluent_sets) noexcept;

    template<formalism::FactKind T>
    [[nodiscard]] const TaggedFactSets<T>& get() const;

    [[nodiscard]] const TaggedFactSets<formalism::StaticTag>& static_sets() const noexcept { return *m_static_sets; }
    [[nodiscard]] const TaggedFactSets<formalism::FluentTag>& fluent_sets() const noexcept { return *m_fluent_sets; }

private:
    const TaggedFactSets<formalism::StaticTag>* m_static_sets;
    const TaggedFactSets<formalism::FluentTag>* m_fluent_sets;
};

}

#endif
