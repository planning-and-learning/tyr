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

#ifndef TYR_DATALOG_ASSIGNMENT_SETS_HPP_
#define TYR_DATALOG_ASSIGNMENT_SETS_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/lifted/assignment.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <limits>
#include <optional>
#include <tuple>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

struct PerfectAssignmentHash
{
    size_t m_num_assignments;
    size_t m_num_objects;
    std::vector<ygg::uint_t> m_remapping;
    std::vector<size_t> m_vertex_offsets;
    std::vector<size_t> m_pair_offsets;

    PerfectAssignmentHash(const analysis::VariableDomainList& parameter_domains, size_t num_objects);

    size_t get_rank(const VertexAssignment& assignment) const noexcept;
    size_t get_rank(const EdgeAssignment& assignment) const noexcept;
    std::optional<size_t> find_rank(const VertexAssignment& assignment) const noexcept;
    std::optional<size_t> find_rank(const EdgeAssignment& assignment) const noexcept;

    size_t size() const noexcept;
};

template<::tyr::formalism::FactKind T>
class PredicateAssignmentSet
{
private:
    ::tyr::formalism::datalog::PredicateView<T> m_predicate;
    ygg::Index<::tyr::formalism::Predicate<T>> m_predicate_index;

    PerfectAssignmentHash m_hash;
    boost::dynamic_bitset<> m_set;

public:
    PredicateAssignmentSet(::tyr::formalism::datalog::PredicateView<T> predicate, const analysis::VariableDomainList& parameter_domains, size_t num_objects);

    void reset() noexcept;

    void insert(::tyr::formalism::datalog::PredicateBindingView<T> binding);

    bool operator[](const VertexAssignment& assignment) const noexcept;
    bool operator[](const EdgeAssignment& assignment) const noexcept;
    bool at(const VertexAssignment& assignment) const noexcept;
    bool at(const EdgeAssignment& assignment) const noexcept;

    size_t size() const noexcept;
    const PerfectAssignmentHash& get_hash() const noexcept;
    const boost::dynamic_bitset<>& get_set() const noexcept;
};

template<::tyr::formalism::FactKind T>
class PredicateAssignmentSets
{
private:
    std::vector<PredicateAssignmentSet<T>> m_sets;

public:
    PredicateAssignmentSets();
    PredicateAssignmentSets(::tyr::formalism::datalog::PredicateListView<T> predicates,
                            const analysis::PredicateDomainMap<T>& predicate_domains,
                            size_t num_objects);

    void reset() noexcept;

    void insert(::tyr::formalism::datalog::GroundAtomView<T> ground_atom);
    void insert(::tyr::formalism::datalog::PredicateBindingView<T> binding);
    void insert(::tyr::formalism::datalog::PredicateBindingForwardRangeView<T> bindings);

    const PredicateAssignmentSet<T>& get_set(ygg::Index<::tyr::formalism::Predicate<T>> index) const noexcept;

    size_t size() const noexcept;

    const std::vector<PredicateAssignmentSet<T>>& get_sets() const noexcept;
};

template<::tyr::formalism::FactKind T>
class FunctionAssignmentSet
{
private:
    ::tyr::formalism::datalog::FunctionView<T> m_function;
    ygg::Index<::tyr::formalism::Function<T>> m_function_index;

    PerfectAssignmentHash m_hash;
    std::vector<ygg::ClosedInterval<ygg::float_t>> m_set;

public:
    FunctionAssignmentSet(::tyr::formalism::datalog::FunctionView<T> function, const analysis::VariableDomainList& parameter_domains, size_t num_objects);

    void reset() noexcept;

    bool insert(::tyr::formalism::datalog::FunctionBindingView<T> binding, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(::tyr::formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value);
    bool insert(::tyr::formalism::datalog::GroundFunctionTermValueView<T> fterm_value);

    ygg::ClosedInterval<ygg::float_t> operator[](const EmptyAssignment& assignment) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](const VertexAssignment& assignment) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](const EdgeAssignment& assignment) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](::tyr::formalism::datalog::FunctionBindingView<T> binding) const noexcept;

    ygg::ClosedInterval<ygg::float_t> at(const EmptyAssignment& assignment) const noexcept;
    ygg::ClosedInterval<ygg::float_t> at(const VertexAssignment& assignment) const noexcept;
    ygg::ClosedInterval<ygg::float_t> at(const EdgeAssignment& assignment) const noexcept;
    ygg::ClosedInterval<ygg::float_t> at(::tyr::formalism::datalog::FunctionBindingView<T> binding) const noexcept;

    size_t size() const noexcept;
    const PerfectAssignmentHash& get_hash() const noexcept;
};

template<::tyr::formalism::FactKind T>
class FunctionAssignmentSets
{
private:
    std::vector<FunctionAssignmentSet<T>> m_sets;

public:
    FunctionAssignmentSets();
    FunctionAssignmentSets(::tyr::formalism::datalog::FunctionListView<T> functions,
                           const analysis::FunctionDomainMap<T>& function_domains,
                           size_t num_objects);

    void reset() noexcept;

    bool insert(::tyr::formalism::datalog::FunctionBindingView<T> binding, ygg::ClosedInterval<ygg::float_t> interval);
    bool insert(::tyr::formalism::datalog::GroundFunctionTermView<T> function_term, ygg::float_t value);
    bool insert(::tyr::formalism::datalog::GroundFunctionTermView<T> function_term, ygg::ClosedInterval<ygg::float_t> interval);
    void insert(::tyr::formalism::datalog::GroundFunctionTermListView<T> function_terms, const std::vector<ygg::float_t>& values);
    void insert(::tyr::formalism::datalog::GroundFunctionTermValueListView<T> fterm_values);

    const FunctionAssignmentSet<T>& get_set(ygg::Index<::tyr::formalism::Function<T>> index) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](::tyr::formalism::datalog::FunctionBindingView<T> binding) const noexcept;
    ygg::ClosedInterval<ygg::float_t> at(::tyr::formalism::datalog::FunctionBindingView<T> binding) const noexcept;
    std::vector<FunctionAssignmentSet<T>>& get_sets() noexcept;
    const std::vector<FunctionAssignmentSet<T>>& get_sets() const noexcept;

    size_t size() const noexcept;
};

template<::tyr::formalism::FactKind T>
struct TaggedAssignmentSets
{
    PredicateAssignmentSets<T> predicate;
    FunctionAssignmentSets<T> function;

    TaggedAssignmentSets();
    TaggedAssignmentSets(::tyr::formalism::datalog::PredicateListView<T> predicates,
                         ::tyr::formalism::datalog::FunctionListView<T> functions,
                         const analysis::PredicateDomainMap<T>& predicate_domains,
                         const analysis::FunctionDomainMap<T>& function_domains,
                         size_t num_objects);
    TaggedAssignmentSets(::tyr::formalism::datalog::PredicateListView<T> predicates,
                         ::tyr::formalism::datalog::FunctionListView<T> functions,
                         const analysis::PredicateDomainMap<T>& predicate_domains,
                         const analysis::FunctionDomainMap<T>& function_domains,
                         size_t num_objects,
                         const TaggedFactSets<T>& fact_sets);

    void insert(const TaggedFactSets<T>& fact_sets);

    void reset() noexcept;
};

struct AssignmentSets
{
    const TaggedAssignmentSets<::tyr::formalism::StaticTag>& static_sets;
    const TaggedAssignmentSets<::tyr::formalism::FluentTag>& fluent_sets;

    AssignmentSets(const TaggedAssignmentSets<::tyr::formalism::StaticTag>& static_sets, const TaggedAssignmentSets<::tyr::formalism::FluentTag>& fluent_sets);

    template<::tyr::formalism::FactKind T>
    const TaggedAssignmentSets<T>& get() const noexcept;
};

}

#endif
