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

#include "tyr/datalog/fact_sets_iterators.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <boost/dynamic_bitset.hpp>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::datalog
{

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
    bool insert(::tyr::formalism::datalog::GroundAtomView<T> ground_atom);
    bool insert(::tyr::formalism::datalog::PredicateBindingView<T> binding);
    bool insert(::tyr::formalism::datalog::PredicateBindingForwardRangeView<T> bindings);
    bool insert(const std::vector<::tyr::formalism::datalog::PredicateBindingView<T>>& bindings);

    bool contains(::tyr::formalism::datalog::PredicateBindingView<T> binding) const noexcept;

    PredicateBindingIndexRange<T> get_binding_indices() const noexcept;
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
    bool insert(::tyr::formalism::datalog::GroundAtomView<T> ground_atom);
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
    bool insert(::tyr::formalism::datalog::GroundFunctionTermView<T> fterm, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(::tyr::formalism::datalog::GroundFunctionTermView<T> fterm, ygg::float_t value);
    bool insert(::tyr::formalism::datalog::GroundFunctionTermValueView<T> fterm_value);
    bool insert(::tyr::formalism::datalog::GroundFunctionTermValueListView<T> fterm_values);

    ygg::ClosedInterval<ygg::float_t> operator[](::tyr::formalism::datalog::FunctionBindingView<T> binding) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](::tyr::formalism::datalog::GroundFunctionTermView<T> fterm) const noexcept;

    FunctionBindingIndexValueRange<T> get_binding_index_values() const noexcept;
    FunctionBindingViewValueRange<T> get_binding_values() const noexcept;
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
    bool insert(::tyr::formalism::datalog::GroundFunctionTermView<T> function_term, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(::tyr::formalism::datalog::GroundFunctionTermView<T> function_term, ygg::float_t value);
    bool insert(::tyr::formalism::datalog::GroundFunctionTermListView<T> function_terms, const std::vector<ygg::float_t>& values);
    bool insert(::tyr::formalism::datalog::GroundFunctionTermValueView<T> fterm_value);
    bool insert(::tyr::formalism::datalog::GroundFunctionTermValueListView<T> fterm_values);

    ygg::ClosedInterval<ygg::float_t> operator[](::tyr::formalism::datalog::FunctionBindingView<T> binding) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](::tyr::formalism::datalog::GroundFunctionTermView<T> fterm) const noexcept;

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
                   ::tyr::formalism::datalog::GroundAtomListView<T> atoms,
                   ::tyr::formalism::datalog::GroundFunctionTermValueListView<T> fterm_values,
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
