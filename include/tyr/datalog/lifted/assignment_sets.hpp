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
#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <tuple>
#include <urd/repr/containers/bitset_span.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <urd/elements/flat.hpp>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{

struct PerfectAssignmentHash
{
    size_t m_num_assignments = 1;
    size_t m_num_objects;
    urd::Store<urd::sequence<ygg::uint_t>> m_remapping;
    urd::Store<urd::sequence<size_t>> m_vertex_offsets;
    urd::Store<urd::sequence<size_t>> m_pair_offsets;

    PerfectAssignmentHash(const analysis::VariableDomainList& parameter_domains, size_t num_objects);

    [[nodiscard]] size_t get_rank(const VertexAssignment& assignment) const noexcept;
    [[nodiscard]] size_t get_rank(const EdgeAssignment& assignment) const noexcept;
    [[nodiscard]] std::optional<size_t> find_rank(const VertexAssignment& assignment) const noexcept;
    [[nodiscard]] std::optional<size_t> find_rank(const EdgeAssignment& assignment) const noexcept;

    [[nodiscard]] size_t size() const noexcept;
};

template<formalism::FactKind T>
class PredicateAssignmentSet
{
private:
    urd::index<formalism::Predicate<T>> m_predicate_index;

    PerfectAssignmentHash m_hash;
    /// The ranks of the assignments held: a bitset that clears only the words it set.
    urd::Store<urd::set<size_t>, urd::dense, urd::proportional_clear> m_ranks;
    urd::Store<urd::sequence<ygg::uint_t>> m_remapped_objects;

public:
    PredicateAssignmentSet(formalism::datalog::PredicateView<T> predicate, const analysis::VariableDomainList& parameter_domains, size_t num_objects);

    void reset() noexcept;

    void insert(formalism::datalog::PredicateBindingView<T> binding);

    bool operator[](const VertexAssignment& assignment) const noexcept;
    bool operator[](const EdgeAssignment& assignment) const noexcept;
    [[nodiscard]] bool at(const VertexAssignment& assignment) const noexcept;
    [[nodiscard]] bool at(const EdgeAssignment& assignment) const noexcept;

    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] const PerfectAssignmentHash& get_hash() const noexcept;
    [[nodiscard]] const urd::Store<urd::set<size_t>, urd::dense, urd::proportional_clear>& get_set() const noexcept;
};

template<formalism::FactKind T>
class PredicateAssignmentSets
{
private:
    urd::Store<urd::sequence<PredicateAssignmentSet<T>>> m_sets;

public:
    PredicateAssignmentSets();
    PredicateAssignmentSets(formalism::datalog::PredicateListView<T> predicates, const analysis::PredicateDomainMap<T>& predicate_domains, size_t num_objects);

    void reset() noexcept;

    void insert(formalism::datalog::AtomView<GroundTag, T> ground_atom);
    void insert(formalism::datalog::PredicateBindingView<T> binding);
    void insert(formalism::datalog::PredicateBindingForwardRangeView<T> bindings);

    [[nodiscard]] const PredicateAssignmentSet<T>& get_set(urd::index<formalism::Predicate<T>> index) const noexcept;

    [[nodiscard]] size_t size() const noexcept;

    [[nodiscard]] const urd::Store<urd::sequence<PredicateAssignmentSet<T>>>& get_sets() const noexcept;
};

template<formalism::FactKind T>
class FunctionAssignmentSet
{
private:
    urd::index<formalism::Function<T>> m_function_index;

    PerfectAssignmentHash m_hash;
    urd::Store<urd::sequence<ygg::ClosedInterval<ygg::float_t>>> m_set;
    urd::Store<urd::sequence<ygg::uint_t>> m_remapped_objects;

public:
    FunctionAssignmentSet(formalism::datalog::FunctionView<T> function, const analysis::VariableDomainList& parameter_domains, size_t num_objects);

    void reset() noexcept;

    bool insert(formalism::datalog::FunctionBindingView<T> binding, const ygg::ClosedInterval<ygg::float_t>& interval);
    bool insert(formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermValueView<GroundTag, T> fterm_value);

    ygg::ClosedInterval<ygg::float_t> operator[](const EmptyAssignment& assignment) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](const VertexAssignment& assignment) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](const EdgeAssignment& assignment) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingView<T> binding) const;

    [[nodiscard]] ygg::ClosedInterval<ygg::float_t> at(const EmptyAssignment& assignment) const noexcept;
    [[nodiscard]] ygg::ClosedInterval<ygg::float_t> at(const VertexAssignment& assignment) const noexcept;
    [[nodiscard]] ygg::ClosedInterval<ygg::float_t> at(const EdgeAssignment& assignment) const noexcept;
    [[nodiscard]] ygg::ClosedInterval<ygg::float_t> at(formalism::datalog::FunctionBindingView<T> binding) const;

    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] const PerfectAssignmentHash& get_hash() const noexcept;
};

template<formalism::FactKind T>
class FunctionAssignmentSets
{
private:
    urd::Store<urd::sequence<FunctionAssignmentSet<T>>> m_sets;

public:
    FunctionAssignmentSets();
    FunctionAssignmentSets(formalism::datalog::FunctionListView<T> functions, const analysis::FunctionDomainMap<T>& function_domains, size_t num_objects);

    void reset() noexcept;

    bool insert(formalism::datalog::FunctionBindingView<T> binding, const ygg::ClosedInterval<ygg::float_t>& interval);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, const ygg::ClosedInterval<ygg::float_t>& interval);
    void insert(formalism::datalog::FunctionTermListView<GroundTag, T> function_terms, const urd::Store<urd::sequence<ygg::float_t>>& values);
    void insert(formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values);

    [[nodiscard]] const FunctionAssignmentSet<T>& get_set(urd::index<formalism::Function<T>> index) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](formalism::datalog::FunctionBindingView<T> binding) const;
    [[nodiscard]] ygg::ClosedInterval<ygg::float_t> at(formalism::datalog::FunctionBindingView<T> binding) const;
    urd::Store<urd::sequence<FunctionAssignmentSet<T>>>& get_sets() noexcept;
    [[nodiscard]] const urd::Store<urd::sequence<FunctionAssignmentSet<T>>>& get_sets() const noexcept;

    [[nodiscard]] size_t size() const noexcept;
};

template<formalism::FactKind T>
struct TaggedAssignmentSets
{
    PredicateAssignmentSets<T> predicate;
    FunctionAssignmentSets<T> function;

    TaggedAssignmentSets();
    TaggedAssignmentSets(formalism::datalog::PredicateListView<T> predicates,
                         formalism::datalog::FunctionListView<T> functions,
                         const analysis::PredicateDomainMap<T>& predicate_domains,
                         const analysis::FunctionDomainMap<T>& function_domains,
                         size_t num_objects);
    TaggedAssignmentSets(formalism::datalog::PredicateListView<T> predicates,
                         formalism::datalog::FunctionListView<T> functions,
                         const analysis::PredicateDomainMap<T>& predicate_domains,
                         const analysis::FunctionDomainMap<T>& function_domains,
                         size_t num_objects,
                         const TaggedFactSets<T>& fact_sets);

    void insert(const TaggedFactSets<T>& fact_sets);

    void reset() noexcept;
};

class AssignmentSets
{
public:
    AssignmentSets(const TaggedAssignmentSets<formalism::StaticTag>& static_sets, const TaggedAssignmentSets<formalism::FluentTag>& fluent_sets);

    template<formalism::FactKind T>
    const TaggedAssignmentSets<T>& get() const noexcept;

    [[nodiscard]] const TaggedAssignmentSets<formalism::StaticTag>& static_sets() const noexcept { return *m_static_sets; }
    [[nodiscard]] const TaggedAssignmentSets<formalism::FluentTag>& fluent_sets() const noexcept { return *m_fluent_sets; }

private:
    const TaggedAssignmentSets<formalism::StaticTag>* m_static_sets;
    const TaggedAssignmentSets<formalism::FluentTag>* m_fluent_sets;
};

}

#endif
