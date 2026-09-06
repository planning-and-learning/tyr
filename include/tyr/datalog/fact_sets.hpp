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

#include <boost/dynamic_bitset.hpp>
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

class PredicateFactRowIterator
{
public:
    using value_type = ygg::Index<::tyr::formalism::Row>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    PredicateFactRowIterator() noexcept = default;
    PredicateFactRowIterator(const boost::dynamic_bitset<>& data, bool begin) noexcept :
        m_data(&data),
        m_position(begin ? data.find_first() : boost::dynamic_bitset<>::npos)
    {
    }

    value_type operator*() const noexcept
    {
        assert(m_data);
        return value_type { static_cast<ygg::uint_t>(m_position) };
    }

    PredicateFactRowIterator& operator++() noexcept
    {
        assert(m_data);
        m_position = m_data->find_next(m_position);
        return *this;
    }

    PredicateFactRowIterator operator++(int) noexcept
    {
        auto copy = *this;
        ++(*this);
        return copy;
    }

    friend bool operator==(const PredicateFactRowIterator& lhs, const PredicateFactRowIterator& rhs) noexcept
    {
        return lhs.m_data == rhs.m_data && lhs.m_position == rhs.m_position;
    }

private:
    const boost::dynamic_bitset<>* m_data = nullptr;
    boost::dynamic_bitset<>::size_type m_position = boost::dynamic_bitset<>::npos;
};

using PredicateFactRowRange = std::ranges::subrange<PredicateFactRowIterator>;

template<::tyr::formalism::FactKind T>
using PredicateBindingRange = ::tyr::formalism::RelationBindingsForwardRange<::tyr::formalism::Predicate<T>, PredicateFactRowRange>;

template<::tyr::formalism::FactKind T>
using PredicateBindingRangeView = ygg::View<PredicateBindingRange<T>, ::tyr::formalism::datalog::Repository>;

}

template<::tyr::formalism::FactKind T>
using PredicateBindingViewRange = std::ranges::subrange<typename detail::PredicateBindingRangeView<T>::const_iterator>;

template<::tyr::formalism::FactKind T>
class PredicateFactSet
{
private:
    ::tyr::formalism::datalog::PredicateView<T> m_predicate;
    const ::tyr::formalism::datalog::Repository& m_repository;

    ygg::Index<::tyr::formalism::Predicate<T>> m_predicate_index;

    boost::dynamic_bitset<> m_bitset;

public:
    explicit PredicateFactSet(::tyr::formalism::datalog::PredicateView<T> predicate, const ::tyr::formalism::datalog::Repository& repository);

    auto get_predicate() const noexcept { return m_predicate; }

    void reset() noexcept;

    bool insert(const PredicateFactSet<T>& other);
    bool insert(::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T> ground_atom);
    bool insert(::tyr::formalism::datalog::PredicateBindingView<T> binding);
    bool insert(::tyr::formalism::datalog::PredicateBindingForwardRangeView<T> bindings);
    bool insert(const std::vector<::tyr::formalism::datalog::PredicateBindingView<T>>& bindings);

    bool contains(::tyr::formalism::datalog::PredicateBindingView<T> binding) const noexcept;

    PredicateBindingViewRange<T> get_bindings() const noexcept;
};

template<::tyr::formalism::FactKind T>
class PredicateFactSets
{
private:
    std::vector<PredicateFactSet<T>> m_sets;

public:
    explicit PredicateFactSets(::tyr::formalism::datalog::PredicateListView<T> predicates, const ::tyr::formalism::datalog::Repository& repository);

    void reset() noexcept;

    bool insert(const PredicateFactSets<T>& other);
    bool insert(::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T> ground_atom);
    bool insert(::tyr::formalism::datalog::PredicateBindingView<T> binding);
    bool insert(::tyr::formalism::datalog::PredicateBindingForwardRangeView<T> bindings);

    bool contains(::tyr::formalism::datalog::PredicateBindingView<T> binding) const noexcept;

    const std::vector<PredicateFactSet<T>>& get_sets() const noexcept;
};

template<::tyr::formalism::FactKind T>
class FunctionFactSet
{
private:
    ::tyr::formalism::datalog::FunctionView<T> m_function;
    const ::tyr::formalism::datalog::Repository& m_repository;

    ygg::Index<::tyr::formalism::Function<T>> m_function_index;
    std::vector<ygg::uint_t> m_remap;
    std::vector<ygg::Index<::tyr::formalism::Row>> m_bindings;
    std::vector<ygg::ClosedInterval<ygg::float_t>> m_values;

public:
    explicit FunctionFactSet(::tyr::formalism::datalog::FunctionView<T> function, const ::tyr::formalism::datalog::Repository& repository);

    auto get_function() const noexcept { return m_function; }

    void reset() noexcept;

    bool insert(const FunctionFactSet& other);
    bool insert(::tyr::formalism::datalog::FunctionBindingView<T> binding, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(::tyr::formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value);
    bool insert(::tyr::formalism::datalog::FunctionBindingRandomAccessRangeView<T> bindings, const std::vector<ygg::ClosedInterval<ygg::float_t>>& intervals);
    bool insert(::tyr::formalism::datalog::FunctionBindingRandomAccessRangeView<T> bindings, const std::vector<ygg::float_t>& values);
    bool insert(const std::vector<::tyr::formalism::datalog::FunctionBindingView<T>>& bindings,
                const std::vector<ygg::ClosedInterval<ygg::float_t>>& intervals);
    bool insert(const std::vector<::tyr::formalism::datalog::FunctionBindingView<T>>& bindings, const std::vector<ygg::float_t>& values);
    bool insert(::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T> fterm, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T> fterm, ygg::float_t value);
    bool insert(::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, T> fterm_value);
    bool insert(::tyr::formalism::datalog::FunctionTermValueListView<::tyr::GroundTag, T> fterm_values);

    ygg::ClosedInterval<ygg::float_t> operator[](::tyr::formalism::datalog::FunctionBindingView<T> binding) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T> fterm) const noexcept;

    ::tyr::formalism::datalog::FunctionBindingRandomAccessRangeView<T> get_bindings() const noexcept;
    const std::vector<ygg::ClosedInterval<ygg::float_t>>& get_values() const noexcept;
};

template<::tyr::formalism::FactKind T>
class FunctionFactSets
{
private:
    std::vector<FunctionFactSet<T>> m_sets;

public:
    explicit FunctionFactSets(::tyr::formalism::datalog::FunctionListView<T> functions, const ::tyr::formalism::datalog::Repository& repository);

    void reset() noexcept;

    bool insert(const FunctionFactSets& other);
    bool insert(::tyr::formalism::datalog::FunctionBindingView<T> binding, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(::tyr::formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value);
    bool insert(::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T> function_term, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T> function_term, ygg::float_t value);
    bool insert(::tyr::formalism::datalog::FunctionTermListView<::tyr::GroundTag, T> function_terms, const std::vector<ygg::float_t>& values);
    bool insert(::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, T> fterm_value);
    bool insert(::tyr::formalism::datalog::FunctionTermValueListView<::tyr::GroundTag, T> fterm_values);

    ygg::ClosedInterval<ygg::float_t> operator[](::tyr::formalism::datalog::FunctionBindingView<T> binding) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T> fterm) const noexcept;

    const std::vector<FunctionFactSet<T>>& get_sets() const noexcept;
};

template<::tyr::formalism::FactKind T>
struct TaggedFactSets
{
    PredicateFactSets<T> predicate;
    FunctionFactSets<T> function;

    TaggedFactSets(::tyr::formalism::datalog::PredicateListView<T> predicates,
                   ::tyr::formalism::datalog::FunctionListView<T> functions,
                   const ::tyr::formalism::datalog::Repository& repository);

    TaggedFactSets(::tyr::formalism::datalog::PredicateListView<T> predicates,
                   ::tyr::formalism::datalog::FunctionListView<T> functions,
                   ::tyr::formalism::datalog::AtomListView<::tyr::GroundTag, T> atoms,
                   ::tyr::formalism::datalog::FunctionTermValueListView<::tyr::GroundTag, T> fterm_values,
                   const ::tyr::formalism::datalog::Repository& repository);

    void insert(const TaggedFactSets<T>& other);

    void reset() noexcept;
};

struct FactSets
{
    const TaggedFactSets<::tyr::formalism::StaticTag>& static_sets;
    const TaggedFactSets<::tyr::formalism::FluentTag>& fluent_sets;

    FactSets(const TaggedFactSets<::tyr::formalism::StaticTag>& static_sets, const TaggedFactSets<::tyr::formalism::FluentTag>& fluent_sets) noexcept;

    template<::tyr::formalism::FactKind T>
    const TaggedFactSets<T>& get() const;
};

}

#endif
