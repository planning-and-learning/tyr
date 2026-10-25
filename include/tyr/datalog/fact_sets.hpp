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
#include <utility>
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
    using value_type = ygg::Index<formalism::Row>;
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
    { return lhs.m_data == rhs.m_data && lhs.m_position == rhs.m_position; }

private:
    const boost::dynamic_bitset<>* m_data = nullptr;
    boost::dynamic_bitset<>::size_type m_position = boost::dynamic_bitset<>::npos;
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
    const formalism::datalog::Repository* m_repository;

    ygg::Index<formalism::Predicate<T>> m_predicate_index;

    boost::dynamic_bitset<> m_bitset;

public:
    explicit PredicateFactSet(formalism::datalog::PredicateView<T> predicate, const formalism::datalog::Repository& repository);

    [[nodiscard]] auto get_predicate() const noexcept { return m_predicate; }

    void reset() noexcept;

    bool insert(const PredicateFactSet<T>& other);
    bool insert(formalism::datalog::AtomView<GroundTag, T> ground_atom);
    bool insert(formalism::datalog::PredicateBindingIndex<T> binding);
    bool insert(formalism::datalog::PredicateBindingView<T> binding) { return insert(binding.get_index()); }
    bool insert(formalism::datalog::PredicateBindingForwardRangeView<T> bindings);
    bool insert(const std::vector<formalism::datalog::PredicateBindingView<T>>& bindings);

    [[nodiscard]] bool contains(formalism::datalog::PredicateBindingIndex<T> binding) const noexcept;
    [[nodiscard]] bool contains(formalism::datalog::PredicateBindingView<T> binding) const noexcept { return contains(binding.get_index()); }

    [[nodiscard]] PredicateBindingViewRange<T> get_bindings() const noexcept;
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

    [[nodiscard]] bool contains(formalism::datalog::PredicateBindingIndex<T> binding) const noexcept;
    [[nodiscard]] bool contains(formalism::datalog::PredicateBindingView<T> binding) const noexcept { return contains(binding.get_index()); }

    [[nodiscard]] const std::vector<PredicateFactSet<T>>& get_sets() const noexcept;
};

template<formalism::FactKind T>
class FunctionFactSet
{
private:
    formalism::datalog::FunctionView<T> m_function;
    const formalism::datalog::Repository* m_repository;

    ygg::Index<formalism::Function<T>> m_function_index;
    std::vector<ygg::uint_t> m_remap;
    std::vector<ygg::Index<formalism::Row>> m_bindings;
    std::vector<ygg::ClosedInterval<ygg::float_t>> m_values;

public:
    explicit FunctionFactSet(formalism::datalog::FunctionView<T> function, const formalism::datalog::Repository& repository);

    [[nodiscard]] auto get_function() const noexcept { return m_function; }

    void reset() noexcept;

    bool insert(const FunctionFactSet& other);
    bool insert(formalism::datalog::FunctionBindingIndex<T> binding, const ygg::ClosedInterval<ygg::float_t>& interval);
    bool insert(formalism::datalog::FunctionBindingView<T> binding, ygg::ClosedInterval<ygg::float_t> interval)
    { return insert(binding.get_index(), std::move(interval)); }
    bool insert(formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value);
    bool insert(formalism::datalog::FunctionBindingRandomAccessRangeView<T> bindings, const std::vector<ygg::ClosedInterval<ygg::float_t>>& intervals);
    bool insert(formalism::datalog::FunctionBindingRandomAccessRangeView<T> bindings, const std::vector<ygg::float_t>& values);
    bool insert(const std::vector<formalism::datalog::FunctionBindingView<T>>& bindings, const std::vector<ygg::ClosedInterval<ygg::float_t>>& intervals);
    bool insert(const std::vector<formalism::datalog::FunctionBindingView<T>>& bindings, const std::vector<ygg::float_t>& values);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> fterm, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> fterm, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermValueView<GroundTag, T> fterm_value);
    bool insert(formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values);

    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingIndex<T> binding) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingView<T> binding) const noexcept { return (*this)[binding.get_index()]; }
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionTermView<GroundTag, T> fterm) const noexcept;

    [[nodiscard]] formalism::datalog::FunctionBindingRandomAccessRangeView<T> get_bindings() const noexcept;
    [[nodiscard]] const std::vector<ygg::ClosedInterval<ygg::float_t>>& get_values() const noexcept;
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
    { return insert(binding.get_index(), std::move(interval)); }
    bool insert(formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermListView<GroundTag, T> function_terms, const std::vector<ygg::float_t>& values);
    bool insert(formalism::datalog::FunctionTermValueView<GroundTag, T> fterm_value);
    bool insert(formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values);

    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingIndex<T> binding) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingView<T> binding) const noexcept { return (*this)[binding.get_index()]; }
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionTermView<GroundTag, T> fterm) const noexcept;

    [[nodiscard]] const std::vector<FunctionFactSet<T>>& get_sets() const noexcept;
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
