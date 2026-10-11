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

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/repository.hpp"

#include <boost/dynamic_bitset.hpp>
#include <limits>
#include <stdexcept>
#include <yggdrasil/containers/dynamic_bitset.hpp>

namespace tyr::datalog
{

/**
 * PredicateFactSet
 */

template<formalism::FactKind T>
PredicateFactSet<T>::PredicateFactSet(formalism::datalog::PredicateView<T> predicate, const formalism::datalog::Repository& repository) :
    m_predicate(predicate),
    m_repository(repository),
    m_predicate_index(m_predicate.get_index()),
    m_bitset()
{
}

template<formalism::FactKind T>
void PredicateFactSet<T>::reset() noexcept
{
    m_bitset.reset();
}

template<formalism::FactKind T>
bool PredicateFactSet<T>::insert(const PredicateFactSet<T>& other)
{
    auto changed = false;
    for (const auto binding : other.get_bindings())
        changed |= insert(binding);
    return changed;
}

template<formalism::FactKind T>
bool PredicateFactSet<T>::insert(formalism::datalog::AtomView<GroundTag, T> ground_atom)
{
    return insert(ground_atom.get_row());
}

template<formalism::FactKind T>
bool PredicateFactSet<T>::insert(formalism::datalog::PredicateBindingView<T> binding)
{
    const auto i = ygg::uint_t(binding.get_index().row);

    if (!ygg::test(i, m_bitset))
    {
        ygg::set(i, true, m_bitset);
        return true;
    }

    return false;
}

template<formalism::FactKind T>
bool PredicateFactSet<T>::insert(formalism::datalog::PredicateBindingForwardRangeView<T> bindings)
{
    auto changed = false;
    for (const auto binding : bindings)
        changed |= insert(binding);
    return changed;
}

template<formalism::FactKind T>
bool PredicateFactSet<T>::insert(const std::vector<formalism::datalog::PredicateBindingView<T>>& bindings)
{
    auto changed = false;
    for (const auto binding : bindings)
        changed |= insert(binding);
    return changed;
}

template<formalism::FactKind T>
bool PredicateFactSet<T>::contains(formalism::datalog::PredicateBindingView<T> binding) const noexcept
{
    return ygg::test(ygg::uint_t(binding.get_index().row), m_bitset);
}

template<formalism::FactKind T>
PredicateBindingViewRange<T> PredicateFactSet<T>::get_bindings() const noexcept
{
    auto rows = detail::PredicateFactRowRange { detail::PredicateFactRowIterator { m_bitset, true }, detail::PredicateFactRowIterator { m_bitset, false } };
    auto bindings = ygg::make_view(detail::PredicateBindingRange<T> { m_predicate_index, rows }, m_repository);
    return { bindings.begin(), bindings.end() };
}

template class PredicateFactSet<formalism::StaticTag>;
template class PredicateFactSet<formalism::FluentTag>;

/**
 * PredicateFactSets
 */

template<formalism::FactKind T>
PredicateFactSets<T>::PredicateFactSets(formalism::datalog::PredicateListView<T> predicates, const formalism::datalog::Repository& repository) : m_sets()
{
    /* Validate inputs. */
    for (ygg::uint_t i = 0; i < predicates.size(); ++i)
        assert(ygg::uint_t(predicates[i].get_index()) == i);

    /* Initialize sets. */
    for (const auto predicate : predicates)
        m_sets.emplace_back(PredicateFactSet<T>(predicate, repository));
}

template<formalism::FactKind T>
void PredicateFactSets<T>::reset() noexcept
{
    for (auto& set : m_sets)
        set.reset();
}

template<formalism::FactKind T>
bool PredicateFactSets<T>::insert(const PredicateFactSets<T>& other)
{
    assert(m_sets.size() == other.m_sets.size());

    auto changed = false;
    for (ygg::uint_t i = 0; i < m_sets.size(); ++i)
        changed |= m_sets[i].insert(other.m_sets[i]);
    return changed;
}

template<formalism::FactKind T>
bool PredicateFactSets<T>::insert(formalism::datalog::AtomView<GroundTag, T> ground_atom)
{
    return insert(ground_atom.get_row());
}

template<formalism::FactKind T>
bool PredicateFactSets<T>::insert(formalism::datalog::PredicateBindingView<T> binding)
{
    return m_sets[ygg::uint_t(binding.get_index().relation)].insert(binding);
}

template<formalism::FactKind T>
bool PredicateFactSets<T>::insert(formalism::datalog::PredicateBindingForwardRangeView<T> bindings)
{
    auto changed = false;
    for (const auto binding : bindings)
        changed |= insert(binding);
    return changed;
}

template<formalism::FactKind T>
bool PredicateFactSets<T>::contains(formalism::datalog::PredicateBindingView<T> binding) const noexcept
{
    return m_sets[ygg::uint_t(binding.get_index().relation)].contains(binding);
}

template<formalism::FactKind T>
const std::vector<PredicateFactSet<T>>& PredicateFactSets<T>::get_sets() const noexcept
{
    return m_sets;
}

template class PredicateFactSets<formalism::StaticTag>;
template class PredicateFactSets<formalism::FluentTag>;

/**
 * FunctionFactSet
 */

template<formalism::FactKind T>
FunctionFactSet<T>::FunctionFactSet(formalism::datalog::FunctionView<T> function, const formalism::datalog::Repository& repository) :
    m_function(function),
    m_repository(repository),
    m_function_index(function.get_index()),
    m_bindings(),
    m_values()
{
}

template<formalism::FactKind T>
void FunctionFactSet<T>::reset() noexcept
{
    m_remap.clear();
    m_bindings.clear();
    m_values.clear();
}

template<formalism::FactKind T>
bool FunctionFactSet<T>::insert(const FunctionFactSet& other)
{
    return insert(other.get_bindings(), other.get_values());
}

template<formalism::FactKind T>
bool FunctionFactSet<T>::insert(formalism::datalog::FunctionBindingView<T> binding, ygg::ClosedInterval<ygg::float_t> interval)
{
    const auto i = ygg::uint_t(binding.get_index().row);

    if (i < m_remap.size())
    {
        const auto pos = m_remap[i];
        if (pos != std::numeric_limits<ygg::uint_t>::max())
        {
            const auto old_interval = m_values[pos];
            const auto new_interval = hull(old_interval, interval);
            m_values[pos] = new_interval;
            return old_interval != new_interval;
        }
    }

    const auto pos = ygg::uint_t(m_bindings.size());
    ygg::set(i, pos, m_remap, std::numeric_limits<ygg::uint_t>::max());

    m_bindings.push_back(binding.get_index().row);
    m_values.push_back(interval);
    return true;
}

template<formalism::FactKind T>
bool FunctionFactSet<T>::insert(formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value)
{
    return insert(binding, ygg::ClosedInterval<ygg::float_t>(value, value));
}

template<formalism::FactKind T>
bool FunctionFactSet<T>::insert(formalism::datalog::FunctionTermView<GroundTag, T> fterm, ygg::ClosedInterval<ygg::float_t> interval)
{
    return insert(fterm.get_row(), interval);
}

template<formalism::FactKind T>
bool FunctionFactSet<T>::insert(formalism::datalog::FunctionTermView<GroundTag, T> fterm, ygg::float_t value)
{
    return insert(fterm.get_row(), value);
}

template<formalism::FactKind T>
bool FunctionFactSet<T>::insert(formalism::datalog::FunctionBindingRandomAccessRangeView<T> bindings,
                                const std::vector<ygg::ClosedInterval<ygg::float_t>>& intervals)
{
    assert(bindings.size() == intervals.size());

    auto changed = false;
    for (ygg::uint_t i = 0; i < bindings.size(); ++i)
        changed |= insert(bindings[i], intervals[i]);
    return changed;
}

template<formalism::FactKind T>
bool FunctionFactSet<T>::insert(formalism::datalog::FunctionBindingRandomAccessRangeView<T> bindings, const std::vector<ygg::float_t>& values)
{
    assert(bindings.size() == values.size());

    auto changed = false;
    for (ygg::uint_t i = 0; i < bindings.size(); ++i)
        changed |= insert(bindings[i], values[i]);
    return changed;
}

template<formalism::FactKind T>
bool FunctionFactSet<T>::insert(const std::vector<formalism::datalog::FunctionBindingView<T>>& bindings,
                                const std::vector<ygg::ClosedInterval<ygg::float_t>>& intervals)
{
    assert(bindings.size() == intervals.size());

    auto changed = false;
    for (ygg::uint_t i = 0; i < bindings.size(); ++i)
        changed |= insert(bindings[i], intervals[i]);
    return changed;
}

template<formalism::FactKind T>
bool FunctionFactSet<T>::insert(const std::vector<formalism::datalog::FunctionBindingView<T>>& bindings, const std::vector<ygg::float_t>& values)
{
    assert(bindings.size() == values.size());

    auto changed = false;
    for (ygg::uint_t i = 0; i < bindings.size(); ++i)
        changed |= insert(bindings[i], values[i]);
    return changed;
}

template<formalism::FactKind T>
bool FunctionFactSet<T>::insert(formalism::datalog::FunctionTermValueView<GroundTag, T> fterm_value)
{
    return insert(fterm_value.get_fterm().get_row(), fterm_value.get_value());
}

template<formalism::FactKind T>
bool FunctionFactSet<T>::insert(formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values)
{
    auto changed = false;
    for (const auto fterm_value : fterm_values)
        changed |= insert(fterm_value);
    return changed;
}

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionFactSet<T>::operator[](formalism::datalog::FunctionBindingView<T> binding) const noexcept
{
    const auto row = binding.get_index().row;
    const auto i = ygg::uint_t(row);

    if (i >= m_remap.size())
        return {};

    return ygg::get(m_remap[i], m_values, ygg::ClosedInterval<ygg::float_t>());
}

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionFactSet<T>::operator[](formalism::datalog::FunctionTermView<GroundTag, T> fterm) const noexcept
{
    return (*this)[fterm.get_row()];
}

template<formalism::FactKind T>
formalism::datalog::FunctionBindingRandomAccessRangeView<T> FunctionFactSet<T>::get_bindings() const noexcept
{
    return ygg::make_view(
        formalism::RelationBindingsRandomAccessRange<formalism::Function<T>, std::vector<ygg::Index<formalism::Row>>> { m_function_index, m_bindings },
        m_repository);
}

template<formalism::FactKind T>
const std::vector<ygg::ClosedInterval<ygg::float_t>>& FunctionFactSet<T>::get_values() const noexcept
{
    return m_values;
}

template class FunctionFactSet<formalism::StaticTag>;
template class FunctionFactSet<formalism::FluentTag>;
template class FunctionFactSet<formalism::AuxiliaryTag>;

/**
 * FunctionFactSets
 */

template<formalism::FactKind T>
FunctionFactSets<T>::FunctionFactSets(formalism::datalog::FunctionListView<T> functions, const formalism::datalog::Repository& repository) : m_sets()
{
    /* Validate inputs. */
    for (ygg::uint_t i = 0; i < functions.size(); ++i)
        assert(ygg::uint_t(functions[i].get_index()) == i);

    /* Initialize sets. */
    for (const auto function : functions)
        m_sets.emplace_back(FunctionFactSet<T>(function, repository));
}

template<formalism::FactKind T>
void FunctionFactSets<T>::reset() noexcept
{
    for (auto& set : m_sets)
        set.reset();
}

template<formalism::FactKind T>
bool FunctionFactSets<T>::insert(const FunctionFactSets& other)
{
    assert(m_sets.size() == other.m_sets.size());

    auto changed = false;
    for (ygg::uint_t i = 0; i < m_sets.size(); ++i)
        changed |= m_sets[i].insert(other.m_sets[i]);
    return changed;
}

template<formalism::FactKind T>
bool FunctionFactSets<T>::insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, ygg::ClosedInterval<ygg::float_t> interval)
{
    return m_sets[ygg::uint_t(function_term.get_function().get_index())].insert(function_term, interval);
}

template<formalism::FactKind T>
bool FunctionFactSets<T>::insert(formalism::datalog::FunctionBindingView<T> binding, ygg::ClosedInterval<ygg::float_t> interval)
{
    return m_sets[ygg::uint_t(binding.get_relation().get_index())].insert(binding, interval);
}

template<formalism::FactKind T>
bool FunctionFactSets<T>::insert(formalism::datalog::FunctionBindingView<T> binding, ygg::float_t value)
{
    return insert(binding, ygg::ClosedInterval<ygg::float_t>(value, value));
}

template<formalism::FactKind T>
bool FunctionFactSets<T>::insert(formalism::datalog::FunctionTermView<GroundTag, T> function_term, ygg::float_t value)
{
    return insert(function_term, ygg::ClosedInterval<ygg::float_t>(value, value));
}

template<formalism::FactKind T>
bool FunctionFactSets<T>::insert(formalism::datalog::FunctionTermListView<GroundTag, T> function_terms, const std::vector<ygg::float_t>& values)
{
    assert(function_terms.size() == values.size());

    auto changed = false;
    for (size_t i = 0; i < function_terms.size(); ++i)
        changed |= insert(function_terms[i], values[i]);
    return changed;
}

template<formalism::FactKind T>
bool FunctionFactSets<T>::insert(formalism::datalog::FunctionTermValueView<GroundTag, T> fterm_value)
{
    return m_sets[ygg::uint_t(fterm_value.get_fterm().get_function().get_index())].insert(fterm_value.get_fterm(), fterm_value.get_value());
}

template<formalism::FactKind T>
bool FunctionFactSets<T>::insert(formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values)
{
    auto changed = false;
    for (const auto fterm_value : fterm_values)
        changed |= insert(fterm_value);
    return changed;
}

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionFactSets<T>::operator[](formalism::datalog::FunctionBindingView<T> binding) const noexcept
{
    return m_sets[ygg::uint_t(binding.get_relation().get_index())][binding];
}

template<formalism::FactKind T>
ygg::ClosedInterval<ygg::float_t> FunctionFactSets<T>::operator[](formalism::datalog::FunctionTermView<GroundTag, T> fterm) const noexcept
{
    return (*this)[fterm.get_row()];
}

template<formalism::FactKind T>
const std::vector<FunctionFactSet<T>>& FunctionFactSets<T>::get_sets() const noexcept
{
    return m_sets;
}

template class FunctionFactSets<formalism::StaticTag>;
template class FunctionFactSets<formalism::FluentTag>;
template class FunctionFactSets<formalism::AuxiliaryTag>;

/**
 * TaggedFactSets
 */

template<formalism::FactKind T>
TaggedFactSets<T>::TaggedFactSets(formalism::datalog::PredicateListView<T> predicates,
                                  formalism::datalog::FunctionListView<T> functions,
                                  const formalism::datalog::Repository& repository) :
    predicate(predicates, repository),
    function(functions, repository)
{
}

template<formalism::FactKind T>
TaggedFactSets<T>::TaggedFactSets(formalism::datalog::PredicateListView<T> predicates,
                                  formalism::datalog::FunctionListView<T> functions,
                                  formalism::datalog::AtomListView<GroundTag, T> atoms,
                                  formalism::datalog::FunctionTermValueListView<GroundTag, T> fterm_values,
                                  const formalism::datalog::Repository& repository) :
    TaggedFactSets(predicates, functions, repository)
{
    for (const auto atom : atoms)
        predicate.insert(atom);
    function.insert(fterm_values);
}

template<formalism::FactKind T>
void TaggedFactSets<T>::insert(const TaggedFactSets<T>& other)
{
    predicate.insert(other.predicate);
    function.insert(other.function);
}

template<formalism::FactKind T>
void TaggedFactSets<T>::reset() noexcept
{
    predicate.reset();
    function.reset();
}

template struct TaggedFactSets<formalism::StaticTag>;
template struct TaggedFactSets<formalism::FluentTag>;

/**
 * FactSets
 */

FactSets::FactSets(const TaggedFactSets<formalism::StaticTag>& static_sets, const TaggedFactSets<formalism::FluentTag>& fluent_sets) noexcept :
    static_sets(static_sets),
    fluent_sets(fluent_sets)
{
}

template<formalism::FactKind T>
const TaggedFactSets<T>& FactSets::get() const
{
    if constexpr (std::is_same_v<T, formalism::StaticTag>)
        return static_sets;
    else if constexpr (std::is_same_v<T, formalism::FluentTag>)
        return fluent_sets;
    else
        throw std::logic_error("FactSets stores only static and fluent fact sets.");
}

template const TaggedFactSets<formalism::StaticTag>& FactSets::get<formalism::StaticTag>() const;
template const TaggedFactSets<formalism::FluentTag>& FactSets::get<formalism::FluentTag>() const;
template const TaggedFactSets<formalism::DerivedTag>& FactSets::get<formalism::DerivedTag>() const;
template const TaggedFactSets<formalism::AuxiliaryTag>& FactSets::get<formalism::AuxiliaryTag>() const;
}
