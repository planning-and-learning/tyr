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

#ifndef TYR_PLANNING_GROUND_STATE_VIEW_HPP_
#define TYR_PLANNING_GROUND_STATE_VIEW_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/state_iterators.hpp"
#include "tyr/planning/state_iterators.hpp"
#include "tyr/planning/state_view.hpp"
#include "tyr/planning/task.hpp"

#include <boost/dynamic_bitset.hpp>
#include <tuple>
#include <yggdrasil/containers/shared_object_pool.hpp>

namespace ygg
{
namespace planning = ::tyr::planning;
template<>
struct View<ygg::Index<planning::State<::tyr::GroundTag>>, std::shared_ptr<planning::StateRepository<::tyr::GroundTag>>>
{
public:
    using TaskType = planning::Task<::tyr::GroundTag>;

    View(std::shared_ptr<planning::StateRepository<::tyr::GroundTag>> owner,
         ygg::SharedObjectPoolPtr<Builder<planning::State<::tyr::GroundTag>>> state_builder) noexcept;
    View(const View&);
    View(View&&) noexcept;
    View& operator=(const View&);
    View& operator=(View&&) noexcept;
    ~View();

    ygg::Index<planning::State<::tyr::GroundTag>> get_index() const;

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

    bool test(::tyr::formalism::planning::GroundAtomView<::tyr::formalism::StaticTag> index) const;
    ygg::float_t get(::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::StaticTag> index) const;
    ::tyr::formalism::planning::FDRValue get(::tyr::formalism::planning::FDRVariableView<::tyr::formalism::FluentTag> index) const;
    ygg::float_t get(::tyr::formalism::planning::GroundFunctionTermView<::tyr::formalism::FluentTag> index) const;
    bool test(::tyr::formalism::planning::GroundAtomView<::tyr::formalism::DerivedTag> index) const;

    /**
     * IterableStateConcept
     */

    planning::AtomRange<::tyr::formalism::StaticTag> get_static_atoms() const noexcept;
    planning::FDRFactRange<::tyr::GroundTag, ::tyr::formalism::FluentTag> get_fluent_facts() const noexcept;
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
    const std::shared_ptr<planning::StateRepository<::tyr::GroundTag>>& get_state_repository() const noexcept;
    const Builder<planning::State<::tyr::GroundTag>>& get_state_builder() const noexcept;

    std::tuple<ygg::Index<planning::State<::tyr::GroundTag>>, ygg::uint_t> identifying_members() const noexcept;

    template<::tyr::formalism::FactKind T>
    const boost::dynamic_bitset<>& get_atoms() const noexcept;

    const std::vector<ygg::uint_t>& get_fluent_values() const noexcept;

    template<::tyr::formalism::FactKind T>
    const std::vector<ygg::float_t>& get_numeric_variables() const noexcept;

private:
    std::shared_ptr<planning::StateRepository<::tyr::GroundTag>> m_state_repository;
    ygg::SharedObjectPoolPtr<Builder<planning::State<::tyr::GroundTag>>> m_state_builder;
};

using GroundStateView = ygg::View<ygg::Index<planning::State<::tyr::GroundTag>>, std::shared_ptr<planning::StateRepository<::tyr::GroundTag>>>;

inline auto GroundStateView::get_static_atoms_view() const noexcept
{
    return get_static_atoms() | std::views::transform([context = this->get_repository()](auto id) { return ygg::make_view(id, *context); });
}
inline auto GroundStateView::get_fluent_facts_view() const noexcept
{
    return get_fluent_facts() | std::views::transform([context = this->get_repository()](auto id) { return ygg::make_view(id, *context); });
}
inline auto GroundStateView::get_derived_atoms_view() const noexcept
{
    return get_derived_atoms() | std::views::transform([context = this->get_repository()](auto id) { return ygg::make_view(id, *context); });
}
inline auto GroundStateView::get_static_fterm_values_view() const noexcept
{
    return get_static_fterm_values()
           | std::views::transform([context = this->get_repository()](auto&& pair)
                                   { return std::make_pair(ygg::make_view(pair.first, *context), pair.second); });
}
inline auto GroundStateView::get_fluent_fterm_values_view() const noexcept
{
    return get_fluent_fterm_values()
           | std::views::transform([context = this->get_repository()](auto&& pair)
                                   { return std::make_pair(ygg::make_view(pair.first, *context), pair.second); });
}
}

namespace tyr
{
using GroundStateView = ygg::GroundStateView;
}

#endif
