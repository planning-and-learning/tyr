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

#ifndef TYR_PLANNING_LIFTED_STATE_VIEW_HPP_
#define TYR_PLANNING_LIFTED_STATE_VIEW_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/state_storage/iterators.hpp"
#include "tyr/planning/state_view.hpp"

#include <boost/dynamic_bitset.hpp>
#include <tuple>
#include <yggdrasil/containers/shared_object_pool.hpp>

namespace ygg
{
namespace planning = ::tyr::planning;
template<>
struct View<ygg::Index<planning::State<::tyr::LiftedTag>>, std::shared_ptr<planning::StateRepository<::tyr::LiftedTag>>>
{
public:
    using TaskType = planning::Task<::tyr::LiftedTag>;

    View(std::shared_ptr<planning::StateRepository<::tyr::LiftedTag>> owner,
         ygg::SharedObjectPoolPtr<Builder<planning::State<::tyr::LiftedTag>>, true> state_builder) noexcept;
    View(const View&);
    View(View&&) noexcept;
    View& operator=(const View&);
    View& operator=(View&&) noexcept;
    ~View();

    ygg::Index<planning::State<::tyr::LiftedTag>> get_index() const;

    /**
     * IndexableStateConcept
     */

    bool test(ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::StaticTag>> index) const;
    ygg::float_t get(ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::StaticTag>> index) const;
    ::tyr::formalism::planning::FDRValue get(ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> index) const;
    ygg::float_t get(ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::FluentTag>> index) const;
    bool test(ygg::Index<::tyr::formalism::planning::GroundAtom<::tyr::formalism::DerivedTag>> index) const;

    /**
     * IndexableViewStateConcept
     */

    bool test(::tyr::formalism::planning::GroundAtomView<::tyr::formalism::StaticTag> view) const;
    ygg::float_t get(::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::StaticTag> view) const;
    ::tyr::formalism::planning::FDRValue get(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> view) const;
    ygg::float_t get(::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::FluentTag> view) const;
    bool test(::tyr::formalism::planning::GroundAtomView<::tyr::formalism::DerivedTag> view) const;

    /**
     * IterableStateConcept
     */

    planning::AtomRange<::tyr::formalism::StaticTag> get_static_atoms() const noexcept;
    planning::FDRFactRange<::tyr::LiftedTag, ::tyr::formalism::FluentTag> get_fluent_facts() const noexcept;
    planning::AtomRange<::tyr::formalism::DerivedTag> get_derived_atoms() const noexcept;
    planning::FunctionTermValueRange<::tyr::formalism::StaticTag> get_static_fterm_values() const noexcept;
    planning::FunctionTermValueRange<::tyr::formalism::FluentTag> get_fluent_fterm_values() const noexcept;

    /**
     * IterableStateViewConcept
     */

    auto get_static_atoms_view() const noexcept;
    auto get_fluent_facts_view() const noexcept;
    auto get_derived_atoms_view() const noexcept;
    auto get_static_fterm_values_view() const noexcept;
    auto get_fluent_fterm_values_view() const noexcept;

    /**
     * Getters
     */

    const std::shared_ptr<::tyr::formalism::planning::Repository>& get_repository() const noexcept;
    const std::shared_ptr<planning::StateRepository<::tyr::LiftedTag>>& get_state_repository() const noexcept;
    const Builder<planning::State<::tyr::LiftedTag>>& get_state_builder() const noexcept;

    std::tuple<ygg::Index<planning::State<::tyr::LiftedTag>>, ygg::uint_t> identifying_members() const noexcept;

private:
    // The pooled builder must be released before its owning repository.
    std::shared_ptr<planning::StateRepository<::tyr::LiftedTag>> m_state_repository;
    ygg::SharedObjectPoolPtr<Builder<planning::State<::tyr::LiftedTag>>, true> m_state_builder;
};

using LiftedStateView = ygg::View<ygg::Index<planning::State<::tyr::LiftedTag>>, std::shared_ptr<planning::StateRepository<::tyr::LiftedTag>>>;

inline auto LiftedStateView::get_static_atoms_view() const noexcept
{
    return get_static_atoms() | std::views::transform([context = this->get_repository()](auto id) { return ygg::make_view(id, *context); });
}
inline auto LiftedStateView::get_fluent_facts_view() const noexcept
{
    return get_fluent_facts() | std::views::transform([context = this->get_repository()](auto id) { return ygg::make_view(id, *context); });
}
inline auto LiftedStateView::get_derived_atoms_view() const noexcept
{
    return get_derived_atoms() | std::views::transform([context = this->get_repository()](auto id) { return ygg::make_view(id, *context); });
}
inline auto LiftedStateView::get_static_fterm_values_view() const noexcept
{
    return get_static_fterm_values()
           | std::views::transform([context = this->get_repository()](auto&& pair)
                                   { return std::make_pair(ygg::make_view(pair.first, *context), pair.second); });
}
inline auto LiftedStateView::get_fluent_fterm_values_view() const noexcept
{
    return get_fluent_fterm_values()
           | std::views::transform([context = this->get_repository()](auto&& pair)
                                   { return std::make_pair(ygg::make_view(pair.first, *context), pair.second); });
}
}

namespace tyr
{
using LiftedStateView = ygg::LiftedStateView;
}

#endif
