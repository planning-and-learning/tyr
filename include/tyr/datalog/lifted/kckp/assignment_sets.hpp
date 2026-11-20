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
#include "tyr/analysis/variable_domains.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/lifted/kckp/assignment.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <tuple>
#include <urd/bits.hpp>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/row_view.hpp>

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

    void insert(formalism::datalog::PredicateRowView<T> binding);

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
    PredicateAssignmentSets() = default;

    PredicateAssignmentSets(formalism::datalog::PredicateListView<T> predicates, const analysis::PredicateDomainMap<T>& predicate_domains, size_t num_objects);

    void reset() noexcept;

    void insert(formalism::datalog::AtomView<GroundTag, T> ground_atom);
    void insert(formalism::datalog::PredicateRowView<T> binding);
    void insert(formalism::datalog::PredicateRowForwardRangeView<T> bindings);

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

    bool insert(formalism::datalog::FunctionRowView<T> binding, const ygg::ClosedInterval<ygg::float_t>& interval);
    bool insert(formalism::datalog::FunctionRowView<T> binding, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermValueView<GroundTag, T> fterm_value);

    ygg::ClosedInterval<ygg::float_t> operator[](const EmptyAssignment& assignment) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](const VertexAssignment& assignment) const noexcept;
    ygg::ClosedInterval<ygg::float_t> operator[](const EdgeAssignment& assignment) const noexcept;

    [[nodiscard]] ygg::ClosedInterval<ygg::float_t> at(const EmptyAssignment& assignment) const noexcept;
    [[nodiscard]] ygg::ClosedInterval<ygg::float_t> at(const VertexAssignment& assignment) const noexcept;
    [[nodiscard]] ygg::ClosedInterval<ygg::float_t> at(const EdgeAssignment& assignment) const noexcept;

    [[nodiscard]] size_t size() const noexcept;
    [[nodiscard]] const PerfectAssignmentHash& get_hash() const noexcept;
};

template<formalism::FactKind T>
class FunctionAssignmentSets
{
private:
    urd::Store<urd::sequence<FunctionAssignmentSet<T>>> m_sets;

public:
    /// No function: those of a kind without function facts (has_function_facts).
    FunctionAssignmentSets() = default;

    FunctionAssignmentSets(formalism::datalog::FunctionListView<T> functions, const analysis::FunctionDomainMap<T>& function_domains, size_t num_objects);

    void reset() noexcept;

    bool insert(formalism::datalog::FunctionRowView<T> binding, const ygg::ClosedInterval<ygg::float_t>& interval);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, ygg::float_t value);
    bool insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, const ygg::ClosedInterval<ygg::float_t>& interval);
    void insert(formalism::datalog::FunctionTermListView<GroundTag, T> function_terms, const urd::Store<urd::sequence<ygg::float_t>>& values);
    void insert(formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values);

    [[nodiscard]] const FunctionAssignmentSet<T>& get_set(urd::index<formalism::Function<T>> index) const noexcept;
    urd::Store<urd::sequence<FunctionAssignmentSet<T>>>& get_sets() noexcept;
    [[nodiscard]] const urd::Store<urd::sequence<FunctionAssignmentSet<T>>>& get_sets() const noexcept;

    [[nodiscard]] size_t size() const noexcept;
};

/// The assignment sets of the facts of kind T: of its predicates, and of its functions where the kind has function facts
/// (has_function_facts; none for derived facts).
template<formalism::FactKind T>
struct TaggedAssignmentSets
{
    PredicateAssignmentSets<T> predicate;
    FunctionAssignmentSets<T> function;

    /// No set yet: a member of MutableAssignmentSets before it is built.
    TaggedAssignmentSets() = default;

    /// The sets of the program's predicates (and functions) of kind T over their domains, holding `fact_sets`.
    TaggedAssignmentSets(formalism::datalog::ProgramView<LiftedTag> program,
                         const analysis::ProgramVariableDomains& domains,
                         const TaggedFactSets<T>& fact_sets);

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

template<typename F>
using MutableTaggedAssignmentSets = TaggedAssignmentSets<F>;

/// The assignment sets of each mutable kind (MutableTags), each reached by get<F>().
class MutableAssignmentSets : public MutableFamily<MutableTaggedAssignmentSets>
{
public:
    MutableAssignmentSets() = default;

    /// The sets of the program's predicates (and functions) of each mutable kind, holding `fact_sets`.
    MutableAssignmentSets(formalism::datalog::ProgramView<LiftedTag> program,
                          const analysis::ProgramVariableDomains& domains,
                          const MutableFactSets& fact_sets);

    /// Inserts the fact of kind F.
    template<formalism::FactKind F>
    void insert(formalism::datalog::PredicateRowView<F> binding)
    { this->template get<F>().predicate.insert(binding); }

    void reset() noexcept;
};

/// The assignment sets of every kind: the static ones and those of each mutable kind.
class AssignmentSets
{
public:
    AssignmentSets(const TaggedAssignmentSets<formalism::StaticTag>& static_sets, const MutableAssignmentSets& mutable_sets);

    template<formalism::FactKind T>
    [[nodiscard]] const TaggedAssignmentSets<T>& get() const noexcept
    {
        if constexpr (std::same_as<T, formalism::StaticTag>)
            return *m_static_sets;
        else
            return m_mutable_sets->template get<T>();
    }

private:
    const TaggedAssignmentSets<formalism::StaticTag>* m_static_sets;
    const MutableAssignmentSets* m_mutable_sets;
};

}

#endif
