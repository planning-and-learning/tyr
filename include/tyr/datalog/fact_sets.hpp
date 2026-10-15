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

#include "tyr/datalog/row_storage.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <cassert>
#include <iterator>
#include <ranges>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog
{

namespace detail
{

/// The rows of a RowSet as relation binding rows.
class PredicateFactRowIterator
{
public:
    using value_type = ygg::Index<formalism::Row>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    PredicateFactRowIterator() noexcept = default;
    explicit PredicateFactRowIterator(RowSet::const_iterator it) noexcept : m_it(it) {}

    value_type operator*() const noexcept { return value_type { static_cast<ygg::uint_t>(*m_it) }; }

    PredicateFactRowIterator& operator++() noexcept
    {
        ++m_it;
        return *this;
    }

    PredicateFactRowIterator operator++(int) noexcept
    {
        auto copy = *this;
        ++(*this);
        return copy;
    }

    friend bool operator==(const PredicateFactRowIterator& lhs, const PredicateFactRowIterator& rhs) noexcept { return lhs.m_it == rhs.m_it; }

private:
    RowSet::const_iterator m_it;
};

using PredicateFactRowRange = std::ranges::subrange<PredicateFactRowIterator>;

template<formalism::FactKind T>
using PredicateBindingRange = formalism::RelationBindingsForwardRange<formalism::Predicate<T>, PredicateFactRowRange>;

template<formalism::FactKind T>
using PredicateBindingRangeView = ygg::View<PredicateBindingRange<T>, formalism::datalog::Repository>;

}

template<formalism::FactKind T>
using PredicateBindingViewRange = std::ranges::subrange<typename detail::PredicateBindingRangeView<T>::const_iterator>;

template<formalism::FactKind T>
class PredicateFactSet
{
private:
    formalism::datalog::PredicateView<T> m_predicate;
    const formalism::datalog::Repository& m_repository;

    ygg::Index<formalism::Predicate<T>> m_predicate_index;

    RowSet m_rows;

public:
    explicit PredicateFactSet(formalism::datalog::PredicateView<T> predicate, const formalism::datalog::Repository& repository);

    auto get_predicate() const noexcept { return m_predicate; }

    void reset() noexcept;

    bool insert(const PredicateFactSet<T>& other);
    bool insert(formalism::datalog::AtomView<GroundTag, T> ground_atom);
    bool insert(formalism::datalog::PredicateBindingIndex<T> binding);
    bool insert(formalism::datalog::PredicateBindingView<T> binding) { return insert(binding.get_index()); }
    bool insert(formalism::datalog::PredicateBindingForwardRangeView<T> bindings);
    bool insert(const std::vector<formalism::datalog::PredicateBindingView<T>>& bindings);

    bool contains(formalism::datalog::PredicateBindingIndex<T> binding) const noexcept;
    bool contains(formalism::datalog::PredicateBindingView<T> binding) const noexcept { return contains(binding.get_index()); }

    PredicateBindingViewRange<T> get_bindings() const noexcept;
};

template<formalism::FactKind T>
class PredicateFactSets
{
private:
    std::vector<PredicateFactSet<T>> m_sets;

public:
    explicit PredicateFactSets(formalism::datalog::PredicateListView<T> predicates, const formalism::datalog::Repository& repository);

    void reset() noexcept;

    bool insert(const PredicateFactSets<T>& other);
    bool insert(formalism::datalog::AtomView<GroundTag, T> ground_atom);
    bool insert(formalism::datalog::PredicateBindingIndex<T> binding);
    bool insert(formalism::datalog::PredicateBindingView<T> binding) { return insert(binding.get_index()); }
    bool insert(formalism::datalog::PredicateBindingForwardRangeView<T> bindings);

    bool contains(formalism::datalog::PredicateBindingIndex<T> binding) const noexcept;
    bool contains(formalism::datalog::PredicateBindingView<T> binding) const noexcept { return contains(binding.get_index()); }

    const std::vector<PredicateFactSet<T>>& get_sets() const noexcept;
};

template<formalism::FactKind T>
class FunctionFactSet
{
private:
    formalism::datalog::FunctionView<T> m_function;
    const formalism::datalog::Repository& m_repository;

    ygg::Index<formalism::Function<T>> m_function_index;
    RowMap<ygg::uint_t> m_remap;  ///< Position + 1 of each row's binding in m_bindings, 0 if absent.
    std::vector<ygg::Index<formalism::Row>> m_bindings;
    std::vector<ygg::ClosedInterval<ygg::float_t>> m_values;

public:
    explicit FunctionFactSet(formalism::datalog::FunctionView<T> function, const formalism::datalog::Repository& repository);

    auto get_function() const noexcept { return m_function; }

    void reset() noexcept;

    bool insert(const FunctionFactSet& other);
    bool insert(formalism::datalog::FunctionBindingIndex<T> binding, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(formalism::datalog::FunctionBindingView<T> binding, ygg::ClosedInterval<ygg::float_t> interval)
    {
        return insert(binding.get_index(), interval);
    }
    bool insert(formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value);
    bool insert(formalism::datalog::FunctionBindingRandomAccessRangeView<T> bindings, const std::vector<ygg::ClosedInterval<ygg::float_t>>& intervals);
    bool insert(formalism::datalog::FunctionBindingRandomAccessRangeView<T> bindings, const std::vector<ygg::float_t>& values);
    bool insert(const std::vector<formalism::datalog::FunctionBindingView<T>>& bindings,
                const std::vector<ygg::ClosedInterval<ygg::float_t>>& intervals);
    bool insert(const std::vector<formalism::datalog::FunctionBindingView<T>>& bindings, const std::vector<ygg::float_t>& values);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> fterm, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> fterm, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermValueView<GroundTag, T> fterm_value);
    bool insert(formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values);

    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingIndex<T> binding) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingView<T> binding) const noexcept { return (*this)[binding.get_index()]; }
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionTermView<GroundTag, T> fterm) const noexcept;

    formalism::datalog::FunctionBindingRandomAccessRangeView<T> get_bindings() const noexcept;
    const std::vector<ygg::ClosedInterval<ygg::float_t>>& get_values() const noexcept;
};

template<formalism::FactKind T>
class FunctionFactSets
{
private:
    std::vector<FunctionFactSet<T>> m_sets;

public:
    explicit FunctionFactSets(formalism::datalog::FunctionListView<T> functions, const formalism::datalog::Repository& repository);

    void reset() noexcept;

    bool insert(const FunctionFactSets& other);
    bool insert(formalism::datalog::FunctionBindingIndex<T> binding, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(formalism::datalog::FunctionBindingView<T> binding, ygg::ClosedInterval<ygg::float_t> interval)
    {
        return insert(binding.get_index(), interval);
    }
    bool insert(formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermListView<GroundTag, T> function_terms, const std::vector<ygg::float_t>& values);
    bool insert(formalism::datalog::FunctionTermValueView<GroundTag, T> fterm_value);
    bool insert(formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values);

    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingIndex<T> binding) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingView<T> binding) const noexcept { return (*this)[binding.get_index()]; }
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionTermView<GroundTag, T> fterm) const noexcept;

    const std::vector<FunctionFactSet<T>>& get_sets() const noexcept;
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

struct FactSets
{
    const TaggedFactSets<formalism::StaticTag>& static_sets;
    const TaggedFactSets<formalism::FluentTag>& fluent_sets;

    FactSets(const TaggedFactSets<formalism::StaticTag>& static_sets, const TaggedFactSets<formalism::FluentTag>& fluent_sets) noexcept;

    template<formalism::FactKind T>
    const TaggedFactSets<T>& get() const;
};

}

#endif
