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

#include "tyr/datalog/lifted/assignment_sets.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "tyr/analysis/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/assignment.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/atom_view.hpp"
#include "tyr/formalism/datalog/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conditional_effect_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/function_term_value_view.hpp"
#include "tyr/formalism/datalog/function_term_view.hpp"
#include "tyr/formalism/datalog/literal_view.hpp"
#include "tyr/formalism/datalog/multi_operator_view.hpp"
#include "tyr/formalism/datalog/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/datalog/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <algorithm>
#include <cassert>
#include <concepts>
#include <limits>
#include <numeric>
#include <tuple>
#include <urd/repr/containers/bitset_span.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::datalog
{
namespace
{
template<typename Objects, std::invocable<size_t> Emit>
    requires std::ranges::forward_range<const Objects> && std::ranges::sized_range<const Objects> && std::same_as<std::ranges::range_value_t<const Objects>, urd::index<formalism::Object>>
             && std::convertible_to<std::ranges::range_reference_t<const Objects>, const urd::index<formalism::Object>&>
void for_each_assignment_rank(const PerfectAssignmentHash& hash, const Objects& objects, urd::Store<urd::sequence<ygg::uint_t>>& remapped, Emit emit)
{
    const auto arity = objects.size();
    assert(remapped.size() == arity);
    auto position = size_t { 0 };
    for (const auto object : objects)
    {
        assert(static_cast<ygg::uint_t>(object.value) < hash.m_num_objects);
        remapped[position] = hash.m_remapping[position * hash.m_num_objects + static_cast<ygg::uint_t>(object.value)];
        ++position;
    }

    for (size_t i = 0; i < arity; ++i)
    {
        if (remapped[i] == 0)
            continue;
        emit(hash.m_vertex_offsets[i] + remapped[i] - 1);
        for (size_t j = i + 1; j < arity; ++j)
        {
            if (remapped[j] == 0)
                continue;
            const auto width = hash.m_vertex_offsets[j + 1] - hash.m_vertex_offsets[j];
            emit(hash.m_pair_offsets[i * arity + j] + (remapped[i] - 1) * width + remapped[j] - 1);
        }
    }
}

template<typename T>
bool update_interval(ygg::ClosedInterval<T>& target, const ygg::ClosedInterval<T>& source)
{
    const auto old = target;
    target = hull(target, source);
    return target != old;
}
}

/**
 * PerfectAssignmentHash
 */

PerfectAssignmentHash::PerfectAssignmentHash(const analysis::VariableDomainList& parameter_domains, size_t num_objects) :

    m_num_objects(num_objects),
    m_remapping(parameter_domains.size() * m_num_objects, 0),
    m_vertex_offsets(parameter_domains.size() + 1, 0),
    m_pair_offsets(parameter_domains.size() * parameter_domains.size(), 0)
{
    const auto num_parameters = parameter_domains.size();

    for (ygg::uint_t i = 0; i < num_parameters; ++i)
    {
        m_vertex_offsets[i] = m_num_assignments;

        const auto& parameter_domain = parameter_domains[i];
        auto new_index = ygg::uint_t { 0 };
        for (const auto object_index : parameter_domain.objects)
            m_remapping[i * m_num_objects + static_cast<ygg::uint_t>(object_index.value)] = ++new_index;
        m_num_assignments += parameter_domain.objects.size();
    }
    m_vertex_offsets[num_parameters] = m_num_assignments;

    for (ygg::uint_t i = 0; i < num_parameters; ++i)
        for (ygg::uint_t j = i + 1; j < num_parameters; ++j)
        {
            m_pair_offsets[i * num_parameters + j] = m_num_assignments;
            m_num_assignments += (m_vertex_offsets[i + 1] - m_vertex_offsets[i]) * (m_vertex_offsets[j + 1] - m_vertex_offsets[j]);
        }
}

size_t PerfectAssignmentHash::get_rank(const VertexAssignment& assignment) const noexcept
{
    assert(assignment.is_valid());
    const auto index = assignment.index.value;
    const auto object = static_cast<ygg::uint_t>(assignment.object.value);
    assert(index + 1 < m_vertex_offsets.size());
    assert(object < m_num_objects);

    const auto remapped_object = m_remapping[index * m_num_objects + object];
    assert(remapped_object != 0);

    const auto result = m_vertex_offsets[index] + remapped_object - 1;

    assert(result < m_num_assignments);

    return result;
}

size_t PerfectAssignmentHash::get_rank(const EdgeAssignment& assignment) const noexcept
{
    assert(assignment.is_valid());
    const auto first_index = assignment.first_index.value;
    const auto second_index = assignment.second_index.value;
    const auto first_object = static_cast<ygg::uint_t>(assignment.first_object.value);
    const auto second_object = static_cast<ygg::uint_t>(assignment.second_object.value);
    const auto num_parameters = m_vertex_offsets.size() - 1;
    assert(second_index < num_parameters);
    assert(first_object < m_num_objects);
    assert(second_object < m_num_objects);

    const auto first_remapped_object = m_remapping[first_index * m_num_objects + first_object];
    const auto second_remapped_object = m_remapping[second_index * m_num_objects + second_object];
    assert(first_remapped_object != 0);
    assert(second_remapped_object != 0);

    const auto second_width = m_vertex_offsets[second_index + 1] - m_vertex_offsets[second_index];
    const auto result = m_pair_offsets[first_index * num_parameters + second_index] + (first_remapped_object - 1) * second_width + second_remapped_object - 1;

    assert(result < m_num_assignments);

    return result;
}

std::optional<size_t> PerfectAssignmentHash::find_rank(const VertexAssignment& assignment) const noexcept
{
    assert(assignment.is_valid());
    const auto index = assignment.index.value;
    const auto object = static_cast<ygg::uint_t>(assignment.object.value);
    assert(index + 1 < m_vertex_offsets.size());
    assert(object < m_num_objects);

    const auto remapped_object = m_remapping[index * m_num_objects + object];
    if (remapped_object == 0)
        return std::nullopt;

    return m_vertex_offsets[index] + remapped_object - 1;
}

std::optional<size_t> PerfectAssignmentHash::find_rank(const EdgeAssignment& assignment) const noexcept
{
    assert(assignment.is_valid());
    const auto first_index = assignment.first_index.value;
    const auto second_index = assignment.second_index.value;
    const auto first_object = static_cast<ygg::uint_t>(assignment.first_object.value);
    const auto second_object = static_cast<ygg::uint_t>(assignment.second_object.value);
    const auto num_parameters = m_vertex_offsets.size() - 1;
    assert(second_index < num_parameters);
    assert(first_object < m_num_objects);
    assert(second_object < m_num_objects);

    const auto first_remapped_object = m_remapping[first_index * m_num_objects + first_object];
    const auto second_remapped_object = m_remapping[second_index * m_num_objects + second_object];
    if (first_remapped_object == 0 || second_remapped_object == 0)
        return std::nullopt;

    const auto second_width = m_vertex_offsets[second_index + 1] - m_vertex_offsets[second_index];
    return m_pair_offsets[first_index * num_parameters + second_index] + (first_remapped_object - 1) * second_width + second_remapped_object - 1;
}

size_t PerfectAssignmentHash::size() const noexcept { return m_num_assignments; }

/**
 * PredicateAssignmentSet
 */

template<formalism::FactKind T>
PredicateAssignmentSet<T>::PredicateAssignmentSet(formalism::datalog::PredicateView<T> predicate,
                                                  const analysis::VariableDomainList& parameter_domains,
                                                  size_t num_objects) :
    m_predicate_index(predicate.index()),
    m_hash(PerfectAssignmentHash(parameter_domains, num_objects)),
    m_remapped_objects(predicate.get_arity())
{
}

template<formalism::FactKind T>
void PredicateAssignmentSet<T>::reset() noexcept
{
    m_ranks.clear();
}

template<formalism::FactKind T>
void PredicateAssignmentSet<T>::insert(formalism::datalog::PredicateBindingView<T> binding)
{
    assert(binding.index().relation == m_predicate_index);
    for_each_assignment_rank(m_hash,
                             binding.get_objects().base(),
                             m_remapped_objects,
                             [&](size_t rank) { m_ranks.insert(rank); });
}

template<formalism::FactKind T>
bool PredicateAssignmentSet<T>::operator[](const VertexAssignment& assignment) const noexcept
{
    const auto rank = m_hash.find_rank(assignment);
    return rank && m_ranks.contains(*rank);
}

template<formalism::FactKind T>
bool PredicateAssignmentSet<T>::operator[](const EdgeAssignment& assignment) const noexcept
{
    const auto rank = m_hash.find_rank(assignment);
    return rank && m_ranks.contains(*rank);
}

template<formalism::FactKind T>
bool PredicateAssignmentSet<T>::at(const VertexAssignment& assignment) const noexcept
{ return m_ranks.contains(m_hash.get_rank(assignment)); }

template<formalism::FactKind T>
bool PredicateAssignmentSet<T>::at(const EdgeAssignment& assignment) const noexcept
{ return m_ranks.contains(m_hash.get_rank(assignment)); }

template<formalism::FactKind T>
size_t PredicateAssignmentSet<T>::size() const noexcept
{ return m_hash.size(); }

template<formalism::FactKind T>
const PerfectAssignmentHash& PredicateAssignmentSet<T>::get_hash() const noexcept
{ return m_hash; }

template<formalism::FactKind T>
const urd::Store<urd::set<size_t>, urd::dense, urd::proportional_clear>& PredicateAssignmentSet<T>::get_set() const noexcept
{ return m_ranks; }

template class PredicateAssignmentSet<formalism::StaticTag>;
template class PredicateAssignmentSet<formalism::FluentTag>;

/**
 * PredicateAssignmentSets
 */

template<formalism::FactKind T>
PredicateAssignmentSets<T>::PredicateAssignmentSets(formalism::datalog::PredicateListView<T> predicates,
                                                    const analysis::PredicateDomainMap<T>& predicate_domains,
                                                    size_t num_objects) :
    m_sets()
{
    /* Validate inputs. */
    for (ygg::uint_t i = 0; i < predicates.size(); ++i)
    {
        assert(static_cast<ygg::uint_t>(predicates[i].index().value) == i);
    }

    /* Initialize sets. */
    m_sets.reserve(predicates.size());
    for (const auto predicate : predicates)
        m_sets.emplace_back(PredicateAssignmentSet<T>(predicate, predicate_domains.at(predicate.index()), num_objects));
}

template<formalism::FactKind T>
void PredicateAssignmentSets<T>::reset() noexcept
{
    for (auto& set : m_sets)
        set.reset();
}

template<formalism::FactKind T>
void PredicateAssignmentSets<T>::insert(formalism::datalog::AtomView<GroundTag, T> ground_atom)
{ insert(ground_atom.get_row()); }

template<formalism::FactKind T>
void PredicateAssignmentSets<T>::insert(formalism::datalog::PredicateBindingView<T> binding)
{ m_sets[static_cast<ygg::uint_t>(binding.index().relation.value)].insert(binding); }

template<formalism::FactKind T>
void PredicateAssignmentSets<T>::insert(formalism::datalog::PredicateBindingForwardRangeView<T> bindings)
{
    for (const auto binding : bindings)
        insert(binding);
}

template<formalism::FactKind T>
const PredicateAssignmentSet<T>& PredicateAssignmentSets<T>::get_set(urd::index<formalism::Predicate<T>> index) const noexcept
{ return m_sets[static_cast<ygg::uint_t>(index.value)]; }

template<formalism::FactKind T>
size_t PredicateAssignmentSets<T>::size() const noexcept
{
    return std::accumulate(m_sets.begin(), m_sets.end(), size_t { 0 }, [](auto&& lhs, auto&& rhs) { return lhs + rhs.size(); });
}

template<formalism::FactKind T>
const urd::Store<urd::sequence<PredicateAssignmentSet<T>>>& PredicateAssignmentSets<T>::get_sets() const noexcept
{ return m_sets; }

template class PredicateAssignmentSets<formalism::StaticTag>;
template class PredicateAssignmentSets<formalism::FluentTag>;

/**
 * FunctionAssignmentSet
 */

template<formalism::FactKind T>
FunctionAssignmentSet<T>::FunctionAssignmentSet(formalism::datalog::FunctionView<T> function,
                                                const analysis::VariableDomainList& parameter_domains,
                                                size_t num_objects) :
    m_function_index(function.index()),
    m_hash(PerfectAssignmentHash(parameter_domains, num_objects)),
    m_set(m_hash.size(), ygg::ClosedInterval<ygg::float_t>()),
    m_remapped_objects(function.get_arity())
{
}

template<formalism::FactKind T>
void FunctionAssignmentSet<T>::reset() noexcept
{ std::fill(m_set.begin(), m_set.end(), ygg::ClosedInterval<ygg::float_t>()); }

template<formalism::FactKind T>
bool FunctionAssignmentSet<T>::insert(formalism::datalog::FunctionBindingView<T> binding, const ygg::ClosedInterval<ygg::float_t>& interval)
{
    assert(binding.index().relation == m_function_index);
    auto changed = update_interval(m_set[EmptyAssignment::rank], interval);
    for_each_assignment_rank(m_hash,
                             binding.get_objects().base(),
                             m_remapped_objects,
                             [&](size_t rank) { changed |= update_interval(m_set[rank], interval); });

    return changed;
}

template<formalism::FactKind T>
bool FunctionAssignmentSet<T>::insert(formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value)
{ return insert(binding, ygg::ClosedInterval<ygg::float_t>(value, value)); }

template<formalism::FactKind T>
bool FunctionAssignmentSet<T>::insert(formalism::datalog::FunctionTermValueView<GroundTag, T> fterm_value)
{ return insert(fterm_value.get_fterm().get_row(), fterm_value.get_value()); }

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::operator[](const EmptyAssignment&) const noexcept
{ return m_set[EmptyAssignment::rank]; }

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::operator[](const VertexAssignment& assignment) const noexcept
{
    const auto rank = m_hash.find_rank(assignment);
    return rank ? m_set[*rank] : ygg::ClosedInterval<ygg::float_t> {};
}

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::operator[](const EdgeAssignment& assignment) const noexcept
{
    const auto rank = m_hash.find_rank(assignment);
    return rank ? m_set[*rank] : ygg::ClosedInterval<ygg::float_t> {};
}

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::at(const EmptyAssignment&) const noexcept
{ return m_set[EmptyAssignment::rank]; }

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::at(const VertexAssignment& assignment) const noexcept
{ return m_set[m_hash.get_rank(assignment)]; }

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::at(const EdgeAssignment& assignment) const noexcept
{ return m_set[m_hash.get_rank(assignment)]; }

template<formalism::FactKind T>
size_t FunctionAssignmentSet<T>::size() const noexcept
{ return m_set.size(); }

template<formalism::FactKind T>
const PerfectAssignmentHash& FunctionAssignmentSet<T>::get_hash() const noexcept
{ return m_hash; }

template class FunctionAssignmentSet<formalism::StaticTag>;
template class FunctionAssignmentSet<formalism::FluentTag>;

/**
 * FunctionAssignmentSets
 */

template<formalism::FactKind T>
FunctionAssignmentSets<T>::FunctionAssignmentSets(formalism::datalog::FunctionListView<T> functions,
                                                  const analysis::FunctionDomainMap<T>& function_domains,
                                                  size_t num_objects) :
    m_sets()
{
    /* Validate inputs. */
    for (ygg::uint_t i = 0; i < functions.size(); ++i)
        assert(functions[i].index().value == i);

    /* Initialize sets. */
    m_sets.reserve(functions.size());
    for (const auto function : functions)
        m_sets.emplace_back(FunctionAssignmentSet<T>(function, function_domains.at(function.index()), num_objects));
}

template<formalism::FactKind T>
void FunctionAssignmentSets<T>::reset() noexcept
{
    for (auto& set : m_sets)
        set.reset();
}

template<formalism::FactKind T>
bool FunctionAssignmentSets<T>::insert(formalism::datalog::FunctionBindingView<T> binding, const ygg::ClosedInterval<ygg::float_t>& interval)
{ return m_sets[binding.get_relation().index().value].insert(binding, std::move(interval)); }

template<formalism::FactKind T>
bool FunctionAssignmentSets<T>::insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, ygg::float_t value)
{ return m_sets[function_term.get_function().index().value].insert(function_term.get_row(), value); }

template<formalism::FactKind T>
bool FunctionAssignmentSets<T>::insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, const ygg::ClosedInterval<ygg::float_t>& interval)
{ return m_sets[function_term.get_function().index().value].insert(function_term.get_row(), std::move(interval)); }

template<formalism::FactKind T>
void FunctionAssignmentSets<T>::insert(formalism::datalog::FunctionTermListView<GroundTag, T> function_terms, const urd::Store<urd::sequence<ygg::float_t>>& values)
{
    assert(function_terms.size() == values.size());

    for (size_t i = 0; i < function_terms.size(); ++i)
        insert(function_terms[i], values[i]);
}

template<formalism::FactKind T>
void FunctionAssignmentSets<T>::insert(formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values)
{
    for (const auto fterm_value : fterm_values)
        insert(fterm_value.get_fterm(), fterm_value.get_value());
}

template<formalism::FactKind T>
const FunctionAssignmentSet<T>& FunctionAssignmentSets<T>::get_set(urd::index<formalism::Function<T>> index) const noexcept
{ return m_sets[index.value]; }

template<formalism::FactKind T>
urd::Store<urd::sequence<FunctionAssignmentSet<T>>>& FunctionAssignmentSets<T>::get_sets() noexcept
{ return m_sets; }

template<formalism::FactKind T>
const urd::Store<urd::sequence<FunctionAssignmentSet<T>>>& FunctionAssignmentSets<T>::get_sets() const noexcept
{ return m_sets; }

template<formalism::FactKind T>
size_t FunctionAssignmentSets<T>::size() const noexcept
{
    return std::accumulate(m_sets.begin(), m_sets.end(), size_t { 0 }, [](auto&& lhs, auto&& rhs) { return lhs + rhs.size(); });
}

template class FunctionAssignmentSets<formalism::StaticTag>;
template class FunctionAssignmentSets<formalism::FluentTag>;

/**
 * TaggedAssignmentSets
 */

template<formalism::FactKind T>
TaggedAssignmentSets<T>::TaggedAssignmentSets(formalism::datalog::PredicateListView<T> predicates,
                                              formalism::datalog::FunctionListView<T> functions,
                                              const analysis::PredicateDomainMap<T>& predicate_domains,
                                              const analysis::FunctionDomainMap<T>& function_domains,
                                              size_t num_objects) :
    predicate(predicates, predicate_domains, num_objects),
    function(functions, function_domains, num_objects)
{
}

template<formalism::FactKind T>
TaggedAssignmentSets<T>::TaggedAssignmentSets(formalism::datalog::PredicateListView<T> predicates,
                                              formalism::datalog::FunctionListView<T> functions,
                                              const analysis::PredicateDomainMap<T>& predicate_domains,
                                              const analysis::FunctionDomainMap<T>& function_domains,
                                              size_t num_objects,
                                              const TaggedFactSets<T>& fact_sets) :
    TaggedAssignmentSets(predicates, functions, predicate_domains, function_domains, num_objects)
{ insert(fact_sets); }

template<formalism::FactKind T>
void TaggedAssignmentSets<T>::insert(const TaggedFactSets<T>& fact_sets)
{
    for (const auto& set : fact_sets.predicate.get_sets())
        for (const auto binding : set.get_bindings())
            predicate.insert(binding);

    for (ygg::uint_t i = 0; i < fact_sets.function.get_sets().size(); ++i)
    {
        auto& self = function.get_sets()[i];
        const auto& other = fact_sets.function.get_sets()[i];
        const auto bindings = other.get_bindings();
        const auto& values = other.get_values();

        assert(bindings.size() == values.size());
        for (size_t j = 0; j < bindings.size(); ++j)
            self.insert(bindings[j], values[j]);
    }
}

template<formalism::FactKind T>
void TaggedAssignmentSets<T>::reset() noexcept
{
    predicate.reset();
    function.reset();
}

template struct TaggedAssignmentSets<formalism::StaticTag>;
template struct TaggedAssignmentSets<formalism::FluentTag>;

/**
 * AssignmentSets
 */

AssignmentSets::AssignmentSets(const TaggedAssignmentSets<formalism::StaticTag>& static_sets, const TaggedAssignmentSets<formalism::FluentTag>& fluent_sets) :
    m_static_sets(&static_sets),
    m_fluent_sets(&fluent_sets)
{
}

}
