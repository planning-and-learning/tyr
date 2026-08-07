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

#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <cassert>
#include <limits>
#include <utility>

namespace ygg
{
namespace planning = ::tyr::planning;
using LiftedStateBuilder = Builder<planning::State<::tyr::LiftedTag>>;

ygg::Index<planning::State<::tyr::LiftedTag>> LiftedStateBuilder::get_index() const { return m_index; }

void LiftedStateBuilder::set(ygg::Index<planning::State<::tyr::LiftedTag>> index) { m_index = index; }

::tyr::formalism::planning::FDRValue LiftedStateBuilder::get(ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const
{
    return ::tyr::formalism::planning::FDRValue(ygg::test(ygg::uint_t(index), m_fact_storage.indices));
}

void LiftedStateBuilder::set(ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fact)
{
    assert(ygg::uint_t(fact.value) < 2);  // can only handle binary using bitsets
    const auto index = ygg::uint_t(fact.variable);
    if (fact.value != ::tyr::formalism::planning::FDRValue::none())
    {
        ygg::set(index, true, m_fact_storage.indices);
    }
    else if (index < m_fact_storage.indices.size())
    {
        m_fact_storage.indices.reset(index);
        if (index + 1 == m_fact_storage.indices.size())
            ygg::trim_trailing_zeros(m_fact_storage.indices);
    }
}

ygg::float_t LiftedStateBuilder::get(ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>> index) const
{
    return ygg::get(ygg::uint_t(index), m_numeric_storage.values, std::numeric_limits<ygg::float_t>::quiet_NaN());
}

void LiftedStateBuilder::set(ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>> index, ygg::float_t value)
{
    ygg::set(ygg::uint_t(index),
             ygg::FloatTolerance<ygg::float_t>::canonicalize(value),
             m_numeric_storage.values,
             std::numeric_limits<ygg::float_t>::quiet_NaN());
}

bool LiftedStateBuilder::test(ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> index) const
{
    return ygg::test(ygg::uint_t(index), m_atom_storage.indices);
}

void LiftedStateBuilder::set(ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> index)
{
    ygg::set(ygg::uint_t(index), true, m_atom_storage.indices);
}

::tyr::formalism::planning::FDRValue LiftedStateBuilder::get(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const
{
    return get(view.get_index());
}

void LiftedStateBuilder::set(::tyr::formalism::planning::FDRFactView<::tyr::formalism::FluentTag> view) { set(view.get_data()); }

ygg::float_t LiftedStateBuilder::get(::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::FluentTag> view) const
{
    return get(view.get_index());
}

void LiftedStateBuilder::set(::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::FluentTag> view, ygg::float_t value)
{
    set(view.get_index(), value);
}

bool LiftedStateBuilder::test(::tyr::formalism::planning::GroundAtomView<::tyr::formalism::DerivedTag> view) const { return test(view.get_index()); }

void LiftedStateBuilder::set(::tyr::formalism::planning::GroundAtomView<::tyr::formalism::DerivedTag> view) { set(view.get_index()); }

void LiftedStateBuilder::clear()
{
    m_index = ygg::Index<planning::State<::tyr::LiftedTag>>::max();
    clear_unextended_part();
    clear_extended_part();
}

void LiftedStateBuilder::clear_unextended_part()
{
    m_fact_storage.indices.clear();
    m_numeric_storage.values.clear();
}

void LiftedStateBuilder::clear_extended_part() { m_atom_storage.indices.clear(); }

void LiftedStateBuilder::assign_unextended_part(const LiftedStateBuilder& other)
{
    m_fact_storage = other.m_fact_storage;
    m_numeric_storage = other.m_numeric_storage;
}

void LiftedStateBuilder::swap(LiftedStateBuilder& other) noexcept
{
    using std::swap;
    swap(m_index, other.m_index);
    m_fact_storage.indices.swap(other.m_fact_storage.indices);
    m_atom_storage.indices.swap(other.m_atom_storage.indices);
    m_numeric_storage.values.swap(other.m_numeric_storage.values);
}

planning::FDRFactRange<::tyr::LiftedTag, ::tyr::formalism::FluentTag> LiftedStateBuilder::get_fluent_facts() const noexcept
{
    return planning::FDRFactRange<::tyr::LiftedTag, ::tyr::formalism::FluentTag>(m_fact_storage);
}

planning::AtomRange<::tyr::formalism::DerivedTag> LiftedStateBuilder::get_derived_atoms() const noexcept
{
    return planning::AtomRange<::tyr::formalism::DerivedTag>(m_atom_storage);
}

planning::FunctionTermValueRange<::tyr::formalism::FluentTag> LiftedStateBuilder::get_fluent_fterm_values() const noexcept
{
    return planning::FunctionTermValueRange<::tyr::formalism::FluentTag>(m_numeric_storage);
}

planning::NumericUnpackedStorage<::tyr::LiftedTag>& LiftedStateBuilder::get_numeric_variables() noexcept { return m_numeric_storage; }

const planning::NumericUnpackedStorage<::tyr::LiftedTag>& LiftedStateBuilder::get_numeric_variables() const noexcept { return m_numeric_storage; }

template<::tyr::formalism::FactKind T>
planning::LiftedUnpackedAtomStorage<T>& LiftedStateBuilder::get_atoms() noexcept
{
    if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
        return m_fact_storage;
    else if constexpr (std::same_as<T, ::tyr::formalism::DerivedTag>)
        return m_atom_storage;
}

template<::tyr::formalism::FactKind T>
const planning::LiftedUnpackedAtomStorage<T>& LiftedStateBuilder::get_atoms() const noexcept
{
    if constexpr (std::same_as<T, ::tyr::formalism::FluentTag>)
        return m_fact_storage;
    else if constexpr (std::same_as<T, ::tyr::formalism::DerivedTag>)
        return m_atom_storage;
}

template planning::FactUnpackedStorage<::tyr::LiftedTag>& LiftedStateBuilder::get_atoms<::tyr::formalism::FluentTag>() noexcept;
template planning::AtomUnpackedStorage<::tyr::LiftedTag>& LiftedStateBuilder::get_atoms<::tyr::formalism::DerivedTag>() noexcept;
template const planning::FactUnpackedStorage<::tyr::LiftedTag>& LiftedStateBuilder::get_atoms<::tyr::formalism::FluentTag>() const noexcept;
template const planning::AtomUnpackedStorage<::tyr::LiftedTag>& LiftedStateBuilder::get_atoms<::tyr::formalism::DerivedTag>() const noexcept;

}

namespace ygg
{
namespace planning = ::tyr::planning;

LiftedStateView::View(std::shared_ptr<planning::StateRepository<::tyr::LiftedTag>> owner,
                      ygg::SharedObjectPoolPtr<Builder<planning::State<::tyr::LiftedTag>>, true> state_builder) noexcept :
    m_state_repository(std::move(owner)),
    m_state_builder(std::move(state_builder))
{
}

LiftedStateView::~View() = default;

LiftedStateView::View(const View&) = default;

LiftedStateView::View(View&&) noexcept = default;

LiftedStateView& LiftedStateView::operator=(const View& other)
{
    if (this != &other)
    {
        m_state_builder = other.m_state_builder;
        m_state_repository = other.m_state_repository;
    }
    return *this;
}

LiftedStateView& LiftedStateView::operator=(View&& other) noexcept
{
    if (this != &other)
    {
        m_state_builder = std::move(other.m_state_builder);
        m_state_repository = std::move(other.m_state_repository);
    }
    return *this;
}

ygg::Index<planning::State<::tyr::LiftedTag>> LiftedStateView::get_index() const { return m_state_builder->get_index(); }

std::tuple<ygg::Index<planning::State<::tyr::LiftedTag>>, ygg::uint_t> LiftedStateView::identifying_members() const noexcept
{
    return std::make_tuple(get_index(), m_state_repository->get_storage_identity());
}

::tyr::formalism::planning::FDRValue LiftedStateView::get(ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const
{
    return m_state_builder->get(index);
}

ygg::float_t LiftedStateView::get(ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>> index) const
{
    return m_state_builder->get(index);
}

bool LiftedStateView::test(ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> index) const
{
    return m_state_builder->test(index);
}

const std::shared_ptr<planning::StateRepository<::tyr::LiftedTag>>& LiftedStateView::get_state_repository() const noexcept { return m_state_repository; }

const Builder<planning::State<::tyr::LiftedTag>>& LiftedStateView::get_state_builder() const noexcept { return *m_state_builder; }

bool LiftedStateView::test(::tyr::formalism::planning::GroundAtomView<::tyr::formalism::StaticTag> view) const { return test(view.get_index()); }

ygg::float_t LiftedStateView::get(::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::StaticTag> view) const { return get(view.get_index()); }

::tyr::formalism::planning::FDRValue LiftedStateView::get(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const
{
    return get(view.get_index());
}

ygg::float_t LiftedStateView::get(::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::FluentTag> view) const { return get(view.get_index()); }

bool LiftedStateView::test(::tyr::formalism::planning::GroundAtomView<::tyr::formalism::DerivedTag> view) const { return test(view.get_index()); }

bool LiftedStateView::test(ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::StaticTag>> index) const
{
    return m_state_repository->get_task()->test(index);
}

ygg::float_t LiftedStateView::get(ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::StaticTag>> index) const
{
    return m_state_repository->get_task()->get(index);
}

planning::AtomRange<::tyr::formalism::StaticTag> LiftedStateView::get_static_atoms() const noexcept
{
    return planning::AtomRange<::tyr::formalism::StaticTag>(m_state_repository->get_task()->get_static_atoms_bitset());
}

planning::FDRFactRange<::tyr::LiftedTag, ::tyr::formalism::FluentTag> LiftedStateView::get_fluent_facts() const noexcept
{
    return m_state_builder->get_fluent_facts();
}

planning::AtomRange<::tyr::formalism::DerivedTag> LiftedStateView::get_derived_atoms() const noexcept { return m_state_builder->get_derived_atoms(); }

planning::FunctionTermValueRange<::tyr::formalism::StaticTag> LiftedStateView::get_static_fterm_values() const noexcept
{
    return planning::FunctionTermValueRange<::tyr::formalism::StaticTag>(m_state_repository->get_task()->get_static_numeric_variables());
}

planning::FunctionTermValueRange<::tyr::formalism::FluentTag> LiftedStateView::get_fluent_fterm_values() const noexcept
{
    return m_state_builder->get_fluent_fterm_values();
}

const std::shared_ptr<::tyr::formalism::planning::Repository>& LiftedStateView::get_repository() const noexcept
{
    return m_state_repository->get_task()->get_repository();
}

static_assert(planning::IterableStateConcept<LiftedStateView>);
static_assert(planning::IterableViewStateConcept<LiftedStateView>);
static_assert(planning::IndexableStateConcept<LiftedStateView, ::tyr::LiftedTag>);
static_assert(planning::IndexableViewStateConcept<LiftedStateView, ::tyr::LiftedTag>);

}
