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

#ifndef TYR_SRC_PLANNING_STATE_VIEW_IMPL_HPP_
#define TYR_SRC_PLANNING_STATE_VIEW_IMPL_HPP_

#include "tyr/planning/state_view.hpp"

#include <utility>

namespace ygg
{
namespace planning = ::tyr::planning;

template<::tyr::TaskKind Kind>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::View(
    std::shared_ptr<planning::StateRepository<Kind>> owner,
    SharedObjectPoolPtr<Builder<planning::State<Kind>>, true> state_builder) noexcept :
    m_state_repository(std::move(owner)),
    m_state_builder(std::move(state_builder))
{
}

template<::tyr::TaskKind Kind>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::~View() = default;

template<::tyr::TaskKind Kind>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::View(const View&) = default;

template<::tyr::TaskKind Kind>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::View(View&&) noexcept = default;

template<::tyr::TaskKind Kind>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>&
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::operator=(const View& other)
{
    if (this != &other)
    {
        m_state_builder = other.m_state_builder;
        m_state_repository = other.m_state_repository;
    }
    return *this;
}

template<::tyr::TaskKind Kind>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>&
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::operator=(View&& other) noexcept
{
    if (this != &other)
    {
        m_state_builder = std::move(other.m_state_builder);
        m_state_repository = std::move(other.m_state_repository);
    }
    return *this;
}

template<::tyr::TaskKind Kind>
Index<planning::State<Kind>> View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_index() const
{
    return m_state_builder->get_index();
}

template<::tyr::TaskKind Kind>
std::tuple<Index<planning::State<Kind>>, uint_t>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::identifying_members() const noexcept
{
    return std::make_tuple(get_index(), m_state_repository->get_storage_identity());
}

template<::tyr::TaskKind Kind>
::tyr::formalism::planning::FDRValue View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get(
    Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const
{
    return m_state_builder->get(index);
}

template<::tyr::TaskKind Kind>
float_t View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get(
    Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>> index) const
{
    return m_state_builder->get(index);
}

template<::tyr::TaskKind Kind>
bool View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::test(
    Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> index) const
{
    return m_state_builder->test(index);
}

template<::tyr::TaskKind Kind>
const std::shared_ptr<planning::StateRepository<Kind>>&
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_state_repository() const noexcept
{
    return m_state_repository;
}

template<::tyr::TaskKind Kind>
const Builder<planning::State<Kind>>& View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_state_builder() const noexcept
{
    return *m_state_builder;
}

template<::tyr::TaskKind Kind>
bool View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::test(
    ::tyr::formalism::planning::GroundAtomView<::tyr::formalism::StaticTag> view) const
{
    return test(view.get_index());
}

template<::tyr::TaskKind Kind>
float_t View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get(
    ::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::StaticTag> view) const
{
    return get(view.get_index());
}

template<::tyr::TaskKind Kind>
::tyr::formalism::planning::FDRValue View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get(
    ::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const
{
    return get(view.get_index());
}

template<::tyr::TaskKind Kind>
float_t View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get(
    ::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::FluentTag> view) const
{
    return get(view.get_index());
}

template<::tyr::TaskKind Kind>
bool View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::test(
    ::tyr::formalism::planning::GroundAtomView<::tyr::formalism::DerivedTag> view) const
{
    return test(view.get_index());
}

template<::tyr::TaskKind Kind>
bool View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::test(
    Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::StaticTag>> index) const
{
    return m_state_repository->get_task()->test(index);
}

template<::tyr::TaskKind Kind>
float_t View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get(
    Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::StaticTag>> index) const
{
    return m_state_repository->get_task()->get(index);
}

template<::tyr::TaskKind Kind>
planning::AtomRange<::tyr::formalism::StaticTag>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_static_atoms() const noexcept
{
    return planning::AtomRange<::tyr::formalism::StaticTag>(m_state_repository->get_task()->get_static_atoms_bitset());
}

template<::tyr::TaskKind Kind>
planning::FDRFactRange<Kind, ::tyr::formalism::FluentTag>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_fluent_facts() const noexcept
{
    return m_state_builder->get_fluent_facts();
}

template<::tyr::TaskKind Kind>
planning::AtomRange<::tyr::formalism::DerivedTag>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_derived_atoms() const noexcept
{
    return m_state_builder->get_derived_atoms();
}

template<::tyr::TaskKind Kind>
planning::FunctionTermValueRange<::tyr::formalism::StaticTag>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_static_fterm_values() const noexcept
{
    return planning::FunctionTermValueRange<::tyr::formalism::StaticTag>(m_state_repository->get_task()->get_static_numeric_variables());
}

template<::tyr::TaskKind Kind>
planning::FunctionTermValueRange<::tyr::formalism::FluentTag>
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_fluent_fterm_values() const noexcept
{
    return m_state_builder->get_fluent_fterm_values();
}

template<::tyr::TaskKind Kind>
const std::shared_ptr<::tyr::formalism::planning::Repository>&
View<Index<planning::State<Kind>>, std::shared_ptr<planning::StateRepository<Kind>>>::get_repository() const noexcept
{
    return m_state_repository->get_task()->get_repository();
}

}

#endif
