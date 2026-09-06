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

#include "tyr/analysis/declarations.hpp"
#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/assignment.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"

#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include <cassert>
#include <limits>
#include <tuple>
#include <vector>
#include <yggdrasil/core/closed_interval.hpp>
#include <yggdrasil/core/config.hpp>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::datalog
{
namespace
{
template<typename T>
bool update_interval(ygg::ClosedInterval<T>& target, ygg::ClosedInterval<T> source)
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
    m_num_assignments(1),
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
            m_remapping[i * m_num_objects + ygg::uint_t(object_index)] = ++new_index;
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
    const auto index = ygg::uint_t(assignment.index);
    const auto object = ygg::uint_t(assignment.object);
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
    const auto first_index = ygg::uint_t(assignment.first_index);
    const auto second_index = ygg::uint_t(assignment.second_index);
    const auto first_object = ygg::uint_t(assignment.first_object);
    const auto second_object = ygg::uint_t(assignment.second_object);
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
    const auto index = ygg::uint_t(assignment.index);
    const auto object = ygg::uint_t(assignment.object);
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
    const auto first_index = ygg::uint_t(assignment.first_index);
    const auto second_index = ygg::uint_t(assignment.second_index);
    const auto first_object = ygg::uint_t(assignment.first_object);
    const auto second_object = ygg::uint_t(assignment.second_object);
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

template<::tyr::formalism::FactKind T>
PredicateAssignmentSet<T>::PredicateAssignmentSet(::tyr::formalism::datalog::PredicateView<T> predicate,
                                                  const analysis::VariableDomainList& parameter_domains,
                                                  size_t num_objects) :
    m_predicate(predicate),
    m_predicate_index(predicate.get_index()),
    m_hash(PerfectAssignmentHash(parameter_domains, num_objects)),
    m_set(m_hash.size(), false)
{
}

template<::tyr::formalism::FactKind T>
void PredicateAssignmentSet<T>::reset() noexcept
{
    m_set.reset();
}

template<::tyr::formalism::FactKind T>
void PredicateAssignmentSet<T>::insert(::tyr::formalism::datalog::PredicateBindingView<T> binding)
{
    const auto arity = m_predicate.get_arity();
    const auto objects = binding.get_objects();

    assert(binding.get_index().relation == m_predicate_index);

    for (ygg::uint_t first_index = 0; first_index < arity; ++first_index)
    {
        const auto first_object = objects[first_index];

        if (const auto rank = m_hash.find_rank(VertexAssignment(::tyr::formalism::ParameterIndex(first_index), first_object.get_index())))
            m_set.set(*rank);

        for (ygg::uint_t second_index = first_index + 1; second_index < arity; ++second_index)
        {
            const auto second_object = objects[second_index];

            if (const auto rank = m_hash.find_rank(EdgeAssignment(::tyr::formalism::ParameterIndex(first_index),
                                                                  first_object.get_index(),
                                                                  ::tyr::formalism::ParameterIndex(second_index),
                                                                  second_object.get_index())))
                m_set.set(*rank);
        }
    }
}

template<::tyr::formalism::FactKind T>
bool PredicateAssignmentSet<T>::operator[](const VertexAssignment& assignment) const noexcept
{
    const auto rank = m_hash.find_rank(assignment);
    return rank && m_set.test(*rank);
}

template<::tyr::formalism::FactKind T>
bool PredicateAssignmentSet<T>::operator[](const EdgeAssignment& assignment) const noexcept
{
    const auto rank = m_hash.find_rank(assignment);
    return rank && m_set.test(*rank);
}

template<::tyr::formalism::FactKind T>
bool PredicateAssignmentSet<T>::at(const VertexAssignment& assignment) const noexcept
{
    return m_set.test(m_hash.get_rank(assignment));
}

template<::tyr::formalism::FactKind T>
bool PredicateAssignmentSet<T>::at(const EdgeAssignment& assignment) const noexcept
{
    return m_set.test(m_hash.get_rank(assignment));
}

template<::tyr::formalism::FactKind T>
size_t PredicateAssignmentSet<T>::size() const noexcept
{
    return m_set.size();
}

template<::tyr::formalism::FactKind T>
const PerfectAssignmentHash& PredicateAssignmentSet<T>::get_hash() const noexcept
{
    return m_hash;
}

template<::tyr::formalism::FactKind T>
const boost::dynamic_bitset<>& PredicateAssignmentSet<T>::get_set() const noexcept
{
    return m_set;
}

template class PredicateAssignmentSet<f::StaticTag>;
template class PredicateAssignmentSet<f::FluentTag>;

/**
 * PredicateAssignmentSets
 */

template<::tyr::formalism::FactKind T>
PredicateAssignmentSets<T>::PredicateAssignmentSets()
{
}

template<::tyr::formalism::FactKind T>
PredicateAssignmentSets<T>::PredicateAssignmentSets(::tyr::formalism::datalog::PredicateListView<T> predicates,
                                                    const analysis::PredicateDomainMap<T>& predicate_domains,
                                                    size_t num_objects) :
    m_sets()
{
    /* Validate inputs. */
    for (ygg::uint_t i = 0; i < predicates.size(); ++i)
    {
        assert(ygg::uint_t(predicates[i].get_index()) == i);
    }

    /* Initialize sets. */
    m_sets.reserve(predicates.size());
    for (const auto predicate : predicates)
        m_sets.emplace_back(PredicateAssignmentSet<T>(predicate, predicate_domains.at(predicate.get_index()), num_objects));
}

template<::tyr::formalism::FactKind T>
void PredicateAssignmentSets<T>::reset() noexcept
{
    for (auto& set : m_sets)
        set.reset();
}

template<::tyr::formalism::FactKind T>
void PredicateAssignmentSets<T>::insert(::tyr::formalism::datalog::AtomView<::tyr::GroundTag, T> ground_atom)
{
    insert(ground_atom.get_row());
}

template<::tyr::formalism::FactKind T>
void PredicateAssignmentSets<T>::insert(::tyr::formalism::datalog::PredicateBindingView<T> binding)
{
    m_sets[ygg::uint_t(binding.get_index().relation)].insert(binding);
}

template<::tyr::formalism::FactKind T>
void PredicateAssignmentSets<T>::insert(::tyr::formalism::datalog::PredicateBindingForwardRangeView<T> bindings)
{
    for (const auto binding : bindings)
        insert(binding);
}

template<::tyr::formalism::FactKind T>
const PredicateAssignmentSet<T>& PredicateAssignmentSets<T>::get_set(ygg::Index<::tyr::formalism::Predicate<T>> index) const noexcept
{
    return m_sets[ygg::uint_t(index)];
}

template<::tyr::formalism::FactKind T>
size_t PredicateAssignmentSets<T>::size() const noexcept
{
    return std::accumulate(m_sets.begin(), m_sets.end(), size_t { 0 }, [](auto&& lhs, auto&& rhs) { return lhs + rhs.size(); });
}

template<::tyr::formalism::FactKind T>
const std::vector<PredicateAssignmentSet<T>>& PredicateAssignmentSets<T>::get_sets() const noexcept
{
    return m_sets;
}

template class PredicateAssignmentSets<f::StaticTag>;
template class PredicateAssignmentSets<f::FluentTag>;

/**
 * FunctionAssignmentSet
 */

template<::tyr::formalism::FactKind T>
FunctionAssignmentSet<T>::FunctionAssignmentSet(::tyr::formalism::datalog::FunctionView<T> function,
                                                const analysis::VariableDomainList& parameter_domains,
                                                size_t num_objects) :
    m_function(function),
    m_function_index(function.get_index()),
    m_hash(PerfectAssignmentHash(parameter_domains, num_objects)),
    m_set(m_hash.size(), ygg::ClosedInterval<ygg::float_t>())
{
}

template<::tyr::formalism::FactKind T>
void FunctionAssignmentSet<T>::reset() noexcept
{
    std::fill(m_set.begin(), m_set.end(), ygg::ClosedInterval<ygg::float_t>());
}

template<::tyr::formalism::FactKind T>
bool FunctionAssignmentSet<T>::insert(::tyr::formalism::datalog::FunctionBindingView<T> binding, ygg::ClosedInterval<ygg::float_t> interval)
{
    const auto objects = binding.get_objects();
    const auto arity = objects.size();
    auto changed = false;

    {
        const auto rank = EmptyAssignment::rank;

        auto& empty_assignment_bound = m_set[rank];
        changed |= update_interval(empty_assignment_bound, interval);
    }

    for (ygg::uint_t first_index = 0; first_index < arity; ++first_index)
    {
        const auto first_object = objects[first_index];

        if (const auto rank = m_hash.find_rank(VertexAssignment(::tyr::formalism::ParameterIndex(first_index), first_object.get_index())))
        {
            auto& single_assignment_bound = m_set[*rank];
            changed |= update_interval(single_assignment_bound, interval);
        }

        for (ygg::uint_t second_index = first_index + 1; second_index < arity; ++second_index)
        {
            const auto second_object = objects[second_index];

            if (const auto rank = m_hash.find_rank(EdgeAssignment(::tyr::formalism::ParameterIndex(first_index),
                                                                  first_object.get_index(),
                                                                  ::tyr::formalism::ParameterIndex(second_index),
                                                                  second_object.get_index())))
            {
                auto& double_assignment_bound = m_set[*rank];
                changed |= update_interval(double_assignment_bound, interval);
            }
        }
    }

    return changed;
}

template<::tyr::formalism::FactKind T>
bool FunctionAssignmentSet<T>::insert(::tyr::formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value)
{
    return insert(binding, ygg::ClosedInterval<ygg::float_t>(value, value));
}

template<::tyr::formalism::FactKind T>
bool FunctionAssignmentSet<T>::insert(::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, T> fterm_value)
{
    return insert(fterm_value.get_fterm().get_row(), fterm_value.get_value());
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::operator[](const EmptyAssignment&) const noexcept
{
    return m_set[EmptyAssignment::rank];
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::operator[](const VertexAssignment& assignment) const noexcept
{
    const auto rank = m_hash.find_rank(assignment);
    return rank ? m_set[*rank] : ygg::ClosedInterval<ygg::float_t> {};
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::operator[](const EdgeAssignment& assignment) const noexcept
{
    const auto rank = m_hash.find_rank(assignment);
    return rank ? m_set[*rank] : ygg::ClosedInterval<ygg::float_t> {};
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::operator[](::tyr::formalism::datalog::FunctionBindingView<T> binding) const noexcept
{
    const auto objects = binding.get_objects();
    const auto arity = objects.size();

    auto result = (*this)[EmptyAssignment()];
    if (empty(result))
        return result;

    for (ygg::uint_t i = 0; i < arity; ++i)
    {
        result = intersect(result, (*this)[VertexAssignment(::tyr::formalism::ParameterIndex(i), objects[i].get_index())]);
        if (empty(result))
            return result;

        for (ygg::uint_t j = i + 1; j < arity; ++j)
        {
            result = intersect(
                result,
                (*this)
                    [EdgeAssignment(::tyr::formalism::ParameterIndex(i), objects[i].get_index(), ::tyr::formalism::ParameterIndex(j), objects[j].get_index())]);
            if (empty(result))
                return result;
        }
    }

    return result;
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::at(const EmptyAssignment&) const noexcept
{
    return m_set[EmptyAssignment::rank];
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::at(const VertexAssignment& assignment) const noexcept
{
    return m_set[m_hash.get_rank(assignment)];
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::at(const EdgeAssignment& assignment) const noexcept
{
    return m_set[m_hash.get_rank(assignment)];
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSet<T>::at(::tyr::formalism::datalog::FunctionBindingView<T> binding) const noexcept
{
    const auto objects = binding.get_objects();
    const auto arity = objects.size();

    auto result = at(EmptyAssignment());
    if (empty(result))
        return result;

    for (ygg::uint_t i = 0; i < arity; ++i)
    {
        result = intersect(result, at(VertexAssignment(::tyr::formalism::ParameterIndex(i), objects[i].get_index())));
        if (empty(result))
            return result;

        for (ygg::uint_t j = i + 1; j < arity; ++j)
        {
            result = intersect(
                result,
                at(EdgeAssignment(::tyr::formalism::ParameterIndex(i), objects[i].get_index(), ::tyr::formalism::ParameterIndex(j), objects[j].get_index())));
            if (empty(result))
                return result;
        }
    }

    return result;
}

template<::tyr::formalism::FactKind T>
size_t FunctionAssignmentSet<T>::size() const noexcept
{
    return m_set.size();
}

template<::tyr::formalism::FactKind T>
const PerfectAssignmentHash& FunctionAssignmentSet<T>::get_hash() const noexcept
{
    return m_hash;
}

template class FunctionAssignmentSet<f::StaticTag>;
template class FunctionAssignmentSet<f::FluentTag>;

/**
 * FunctionAssignmentSets
 */

template<::tyr::formalism::FactKind T>
FunctionAssignmentSets<T>::FunctionAssignmentSets()
{
}

template<::tyr::formalism::FactKind T>
FunctionAssignmentSets<T>::FunctionAssignmentSets(::tyr::formalism::datalog::FunctionListView<T> functions,
                                                  const analysis::FunctionDomainMap<T>& function_domains,
                                                  size_t num_objects) :
    m_sets()
{
    /* Validate inputs. */
    for (ygg::uint_t i = 0; i < functions.size(); ++i)
        assert(functions[i].get_index().get_value() == i);

    /* Initialize sets. */
    m_sets.reserve(functions.size());
    for (const auto function : functions)
        m_sets.emplace_back(FunctionAssignmentSet<T>(function, function_domains.at(function.get_index()), num_objects));
}

template<::tyr::formalism::FactKind T>
void FunctionAssignmentSets<T>::reset() noexcept
{
    for (auto& set : m_sets)
        set.reset();
}

template<::tyr::formalism::FactKind T>
bool FunctionAssignmentSets<T>::insert(::tyr::formalism::datalog::FunctionBindingView<T> binding, ygg::ClosedInterval<ygg::float_t> interval)
{
    return m_sets[binding.get_relation().get_index().get_value()].insert(binding, interval);
}

template<::tyr::formalism::FactKind T>
bool FunctionAssignmentSets<T>::insert(::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T> function_term, ygg::float_t value)
{
    return m_sets[function_term.get_function().get_index().get_value()].insert(function_term.get_row(), value);
}

template<::tyr::formalism::FactKind T>
bool FunctionAssignmentSets<T>::insert(::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, T> function_term, ygg::ClosedInterval<ygg::float_t> interval)
{
    return m_sets[function_term.get_function().get_index().get_value()].insert(function_term.get_row(), interval);
}

template<::tyr::formalism::FactKind T>
void FunctionAssignmentSets<T>::insert(::tyr::formalism::datalog::FunctionTermListView<::tyr::GroundTag, T> function_terms, const std::vector<ygg::float_t>& values)
{
    assert(function_terms.size() == values.size());

    for (size_t i = 0; i < function_terms.size(); ++i)
        insert(function_terms[i], values[i]);
}

template<::tyr::formalism::FactKind T>
void FunctionAssignmentSets<T>::insert(::tyr::formalism::datalog::FunctionTermValueListView<::tyr::GroundTag, T> fterm_values)
{
    for (const auto fterm_value : fterm_values)
        insert(fterm_value.get_fterm(), fterm_value.get_value());
}

template<::tyr::formalism::FactKind T>
const FunctionAssignmentSet<T>& FunctionAssignmentSets<T>::get_set(ygg::Index<::tyr::formalism::Function<T>> index) const noexcept
{
    return m_sets[index.get_value()];
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSets<T>::operator[](::tyr::formalism::datalog::FunctionBindingView<T> binding) const noexcept
{
    return get_set(binding.get_relation().get_index())[binding];
}

template<::tyr::formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionAssignmentSets<T>::at(::tyr::formalism::datalog::FunctionBindingView<T> binding) const noexcept
{
    return get_set(binding.get_relation().get_index()).at(binding);
}

template<::tyr::formalism::FactKind T>
std::vector<FunctionAssignmentSet<T>>& FunctionAssignmentSets<T>::get_sets() noexcept
{
    return m_sets;
}

template<::tyr::formalism::FactKind T>
const std::vector<FunctionAssignmentSet<T>>& FunctionAssignmentSets<T>::get_sets() const noexcept
{
    return m_sets;
}

template<::tyr::formalism::FactKind T>
size_t FunctionAssignmentSets<T>::size() const noexcept
{
    return std::accumulate(m_sets.begin(), m_sets.end(), size_t { 0 }, [](auto&& lhs, auto&& rhs) { return lhs + rhs.size(); });
}

template class FunctionAssignmentSets<f::StaticTag>;
template class FunctionAssignmentSets<f::FluentTag>;

/**
 * TaggedAssignmentSets
 */

template<::tyr::formalism::FactKind T>
TaggedAssignmentSets<T>::TaggedAssignmentSets()
{
}

template<::tyr::formalism::FactKind T>
TaggedAssignmentSets<T>::TaggedAssignmentSets(::tyr::formalism::datalog::PredicateListView<T> predicates,
                                              ::tyr::formalism::datalog::FunctionListView<T> functions,
                                              const analysis::PredicateDomainMap<T>& predicate_domains,
                                              const analysis::FunctionDomainMap<T>& function_domains,
                                              size_t num_objects) :
    predicate(predicates, predicate_domains, num_objects),
    function(functions, function_domains, num_objects)
{
}

template<::tyr::formalism::FactKind T>
TaggedAssignmentSets<T>::TaggedAssignmentSets(::tyr::formalism::datalog::PredicateListView<T> predicates,
                                              ::tyr::formalism::datalog::FunctionListView<T> functions,
                                              const analysis::PredicateDomainMap<T>& predicate_domains,
                                              const analysis::FunctionDomainMap<T>& function_domains,
                                              size_t num_objects,
                                              const TaggedFactSets<T>& fact_sets) :
    TaggedAssignmentSets(predicates, functions, predicate_domains, function_domains, num_objects)
{
    insert(fact_sets);
}

template<::tyr::formalism::FactKind T>
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

template<::tyr::formalism::FactKind T>
void TaggedAssignmentSets<T>::reset() noexcept
{
    predicate.reset();
    function.reset();
}

template struct TaggedAssignmentSets<f::StaticTag>;
template struct TaggedAssignmentSets<f::FluentTag>;

/**
 * AssignmentSets
 */

AssignmentSets::AssignmentSets(const TaggedAssignmentSets<::tyr::formalism::StaticTag>& static_sets,
                               const TaggedAssignmentSets<::tyr::formalism::FluentTag>& fluent_sets) :
    static_sets(static_sets),
    fluent_sets(fluent_sets)
{
}

template<::tyr::formalism::FactKind T>
const TaggedAssignmentSets<T>& AssignmentSets::get() const noexcept
{
    if constexpr (std::is_same_v<T, ::tyr::formalism::StaticTag>)
        return static_sets;
    else if constexpr (std::is_same_v<T, ::tyr::formalism::FluentTag>)
        return fluent_sets;
    else
        static_assert(ygg::dependent_false<T>::value, "Missing case");
}

template const TaggedAssignmentSets<f::StaticTag>& AssignmentSets::get<f::StaticTag>() const noexcept;
template const TaggedAssignmentSets<f::FluentTag>& AssignmentSets::get<f::FluentTag>() const noexcept;

}
