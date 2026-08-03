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

#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/ground/task.hpp"

#include <cassert>
#include <limits>
#include <utility>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

namespace ygg
{
namespace planning = ::tyr::planning;
using GroundStateBuilder = Builder<planning::State<::tyr::GroundTag>>;

ygg::Index<planning::State<::tyr::GroundTag>> GroundStateBuilder::get_index() const { return m_index; }

void GroundStateBuilder::set(ygg::Index<planning::State<::tyr::GroundTag>> index) { m_index = index; }

fp::FDRValue GroundStateBuilder::get(ygg::Index<fp::FDRVariable<f::FluentTag>> index) const
{
    assert(ygg::uint_t(index) < m_fact_storage.values.size());
    return fp::FDRValue(m_fact_storage.values[ygg::uint_t(index)]);
}

fp::FDRValue GroundStateBuilder::get(fp::FDRVariableView<f::FluentTag> view) const { return get(view.get_index()); }

void GroundStateBuilder::set(ygg::Data<fp::FDRFact<f::FluentTag>> fact)
{
    assert(ygg::uint_t(fact.variable) < m_fact_storage.values.size());
    m_fact_storage.values[ygg::uint_t(fact.variable)] = ygg::uint_t(fact.value);
}

ygg::float_t GroundStateBuilder::get(ygg::Index<fp::GroundFunctionTerm<f::FluentTag>> index) const
{
    return ygg::get(ygg::uint_t(index), m_numeric_storage.values, std::numeric_limits<ygg::float_t>::quiet_NaN());
}

void GroundStateBuilder::set(ygg::Index<fp::GroundFunctionTerm<f::FluentTag>> index, ygg::float_t value)
{
    ygg::set(ygg::uint_t(index), value, m_numeric_storage.values, std::numeric_limits<ygg::float_t>::quiet_NaN());
}

bool GroundStateBuilder::test(ygg::Index<fp::GroundAtom<f::DerivedTag>> index) const
{
    assert(ygg::uint_t(index) < m_atom_storage.indices.size());
    return m_atom_storage.indices.test(ygg::uint_t(index));
}

void GroundStateBuilder::set(ygg::Index<fp::GroundAtom<f::DerivedTag>> index)
{
    assert(ygg::uint_t(index) < m_atom_storage.indices.size());
    m_atom_storage.indices.set(ygg::uint_t(index));
}

bool GroundStateBuilder::test(fp::GroundAtomView<f::DerivedTag> view) const { return test(view.get_index()); }

void GroundStateBuilder::set(fp::GroundAtomView<f::DerivedTag> view) { set(view.get_index()); }

void GroundStateBuilder::clear()
{
    clear_unextended_part();
    clear_extended_part();
}

void GroundStateBuilder::clear_unextended_part()
{
    m_fact_storage.values.clear();
    m_numeric_storage.values.clear();
}

void GroundStateBuilder::clear_extended_part() { m_atom_storage.indices.clear(); }

void GroundStateBuilder::assign_unextended_part(const GroundStateBuilder& other)
{
    m_fact_storage = other.m_fact_storage;
    m_numeric_storage = other.m_numeric_storage;
}

void GroundStateBuilder::swap(GroundStateBuilder& other) noexcept
{
    using std::swap;
    swap(m_index, other.m_index);
    m_fact_storage.values.swap(other.m_fact_storage.values);
    m_atom_storage.indices.swap(other.m_atom_storage.indices);
    m_numeric_storage.values.swap(other.m_numeric_storage.values);
}

void GroundStateBuilder::resize_fluent_facts(size_t num_fluent_facts) { m_fact_storage.values.resize(num_fluent_facts, 0); }

void GroundStateBuilder::resize_derived_atoms(size_t num_derived_atoms) { m_atom_storage.indices.resize(num_derived_atoms, false); }

planning::NumericUnpackedStorage<::tyr::GroundTag>& GroundStateBuilder::get_numeric_variables() noexcept { return m_numeric_storage; }

const planning::NumericUnpackedStorage<::tyr::GroundTag>& GroundStateBuilder::get_numeric_variables() const noexcept { return m_numeric_storage; }

template<f::FactKind T>
planning::GroundUnpackedAtomStorage<T>& GroundStateBuilder::get_atoms() noexcept
{
    if constexpr (std::same_as<T, f::FluentTag>)
        return m_fact_storage;
    else if constexpr (std::same_as<T, f::DerivedTag>)
        return m_atom_storage;
}

template<f::FactKind T>
const planning::GroundUnpackedAtomStorage<T>& GroundStateBuilder::get_atoms() const noexcept
{
    if constexpr (std::same_as<T, f::FluentTag>)
        return m_fact_storage;
    else if constexpr (std::same_as<T, f::DerivedTag>)
        return m_atom_storage;
}

template planning::FactUnpackedStorage<::tyr::GroundTag>& GroundStateBuilder::get_atoms<f::FluentTag>() noexcept;
template planning::AtomUnpackedStorage<::tyr::GroundTag>& GroundStateBuilder::get_atoms<f::DerivedTag>() noexcept;
template const planning::FactUnpackedStorage<::tyr::GroundTag>& GroundStateBuilder::get_atoms<f::FluentTag>() const noexcept;
template const planning::AtomUnpackedStorage<::tyr::GroundTag>& GroundStateBuilder::get_atoms<f::DerivedTag>() const noexcept;

}

namespace ygg
{
namespace planning = ::tyr::planning;

GroundStateView::View(std::shared_ptr<planning::StateRepository<::tyr::GroundTag>> owner,
                      ygg::SharedObjectPoolPtr<Builder<planning::State<::tyr::GroundTag>>> state_builder) noexcept :
    m_state_repository(std::move(owner)),
    m_state_builder(std::move(state_builder))
{
}

GroundStateView::~View() = default;

GroundStateView::View(const View&) = default;

GroundStateView::View(View&&) noexcept = default;

GroundStateView& GroundStateView::operator=(const View&) = default;

GroundStateView& GroundStateView::operator=(View&&) noexcept = default;

ygg::Index<planning::State<::tyr::GroundTag>> GroundStateView::get_index() const { return m_state_builder->get_index(); }

std::tuple<ygg::Index<planning::State<::tyr::GroundTag>>, ygg::uint_t> GroundStateView::identifying_members() const noexcept
{
    return std::make_tuple(get_index(), m_state_repository->get_index());
}

::tyr::formalism::planning::FDRValue GroundStateView::get(ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const
{
    return m_state_builder->get(index);
}

ygg::float_t GroundStateView::get(ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>> index) const
{
    return m_state_builder->get(index);
}

bool GroundStateView::test(ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> index) const
{
    return m_state_builder->test(index);
}

const std::shared_ptr<planning::StateRepository<::tyr::GroundTag>>& GroundStateView::get_state_repository() const noexcept { return m_state_repository; }

const Builder<planning::State<::tyr::GroundTag>>& GroundStateView::get_state_builder() const noexcept { return *m_state_builder; }

const std::vector<ygg::uint_t>& GroundStateView::get_fluent_values() const noexcept { return m_state_builder->get_atoms<::tyr::formalism::FluentTag>().values; }

bool GroundStateView::test(::tyr::formalism::planning::GroundAtomView<::tyr::formalism::StaticTag> view) const { return test(view.get_index()); }

ygg::float_t GroundStateView::get(::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::StaticTag> view) const { return get(view.get_index()); }

::tyr::formalism::planning::FDRValue GroundStateView::get(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const
{
    return get(view.get_index());
}

ygg::float_t GroundStateView::get(::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::FluentTag> view) const { return get(view.get_index()); }

bool GroundStateView::test(::tyr::formalism::planning::GroundAtomView<::tyr::formalism::DerivedTag> view) const { return test(view.get_index()); }

bool GroundStateView::test(ygg::Index<fp::GroundAtom<f::StaticTag>> index) const { return m_state_repository->get_task()->test(index); }

ygg::float_t GroundStateView::get(ygg::Index<fp::GroundFunctionTerm<f::StaticTag>> index) const { return m_state_repository->get_task()->get(index); }

template<f::FactKind T>
const boost::dynamic_bitset<>& GroundStateView::get_atoms() const noexcept
{
    if constexpr (std::is_same_v<T, f::StaticTag>)
        return m_state_repository->get_task()->get_static_atoms_bitset();
    else if constexpr (std::is_same_v<T, f::DerivedTag>)
        return m_state_builder->template get_atoms<f::DerivedTag>().indices;
    else
        static_assert(ygg::dependent_false<T>::value, "Missing case");
}

template const boost::dynamic_bitset<>& GroundStateView::get_atoms<f::StaticTag>() const noexcept;
template const boost::dynamic_bitset<>& GroundStateView::get_atoms<f::DerivedTag>() const noexcept;

template<f::FactKind T>
const std::vector<ygg::float_t>& GroundStateView::get_numeric_variables() const noexcept
{
    if constexpr (std::is_same_v<T, f::StaticTag>)
        return m_state_repository->get_task()->get_static_numeric_variables();
    else if constexpr (std::is_same_v<T, f::FluentTag>)
        return m_state_builder->get_numeric_variables().values;
    else
        static_assert(ygg::dependent_false<T>::value, "Missing case");
}

template const std::vector<ygg::float_t>& GroundStateView::get_numeric_variables<f::StaticTag>() const noexcept;
template const std::vector<ygg::float_t>& GroundStateView::get_numeric_variables<f::FluentTag>() const noexcept;

planning::AtomRange<::tyr::formalism::StaticTag> GroundStateView::get_static_atoms() const noexcept
{
    return planning::AtomRange<::tyr::formalism::StaticTag>(m_state_repository->get_task()->get_static_atoms_bitset());
}

planning::FDRFactRange<::tyr::GroundTag, ::tyr::formalism::FluentTag> GroundStateView::get_fluent_facts() const noexcept
{
    return planning::FDRFactRange<::tyr::GroundTag, ::tyr::formalism::FluentTag>(get_fluent_values());
}

planning::AtomRange<::tyr::formalism::DerivedTag> GroundStateView::get_derived_atoms() const noexcept
{
    return planning::AtomRange<::tyr::formalism::DerivedTag>(get_atoms<::tyr::formalism::DerivedTag>());
}

planning::FunctionTermValueRange<::tyr::formalism::StaticTag> GroundStateView::get_static_fterm_values() const noexcept
{
    return planning::FunctionTermValueRange<::tyr::formalism::StaticTag>(m_state_repository->get_task()->get_static_numeric_variables());
}

planning::FunctionTermValueRange<::tyr::formalism::FluentTag> GroundStateView::get_fluent_fterm_values() const noexcept
{
    return planning::FunctionTermValueRange<::tyr::formalism::FluentTag>(get_numeric_variables<::tyr::formalism::FluentTag>());
}

const std::shared_ptr<::tyr::formalism::planning::Repository>& GroundStateView::get_repository() const noexcept
{
    return m_state_repository->get_task()->get_repository();
}

static_assert(planning::IterableStateConcept<GroundStateView>);
static_assert(planning::IterableViewStateConcept<GroundStateView>);
static_assert(planning::IndexableStateConcept<GroundStateView, ::tyr::GroundTag>);
static_assert(planning::IndexableViewStateConcept<GroundStateView, ::tyr::GroundTag>);
}
