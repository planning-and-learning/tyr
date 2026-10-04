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

#ifndef TYR_FORMALISM_PLANNING_TASK_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_TASK_VIEW_HPP_

#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/domain_index.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/function_term_value_view.hpp"
#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_index.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <yggdrasil/containers/optional.hpp>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/detail/view.hpp>

namespace ygg
{

template<::tyr::TaskKind Kind, ygg::formalism::SymbolContextFor<::tyr::formalism::planning::Task<Kind>> C>
class View<ygg::Index<::tyr::formalism::planning::Task<Kind>>, C> : public ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::Task<Kind>>, C>
{
public:
    View(ygg::Index<::tyr::formalism::planning::Task<Kind>> handle, const C& context) noexcept :
        ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::Task<Kind>>, C>(handle, context)
    {
    }

    const auto& get_name() const noexcept { return this->get_data().name; }
    auto get_domain() const noexcept { return ygg::make_view(this->get_data().domain, *this->m_context); }
    auto get_derived_predicates() const noexcept { return ygg::make_view(this->get_data().derived_predicates, *this->m_context); }
    auto get_objects() const noexcept { return ygg::make_view(this->get_data().objects, *this->m_context); }
    size_t get_num_objects() const noexcept { return get_domain().get_constants().size() + get_objects().size(); }
    template<::tyr::formalism::FactKind T>
    auto get_atoms() const noexcept
    {
        return ygg::make_view(this->get_data().template get_atoms<T>(), *this->m_context);
    }
    template<::tyr::formalism::FactKind T>
    auto get_fterm_values() const noexcept
    {
        return ygg::make_view(this->get_data().template get_fterm_values<T>(), *this->m_context);
    }
    auto get_auxiliary_fterm_value() const noexcept { return ygg::make_view(this->get_data().auxiliary_fterm_value, *this->m_context); }
    auto get_goal() const noexcept { return ygg::make_view(this->get_data().goal, *this->m_context); }
    auto get_metric() const noexcept { return ygg::make_view(this->get_data().metric, *this->m_context); }
    auto get_axioms() const noexcept { return ygg::make_view(this->get_data().axioms, *this->m_context); }

    auto get_fluent_variables() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    {
        return ygg::make_view(this->get_data().fluent_variables, *this->m_context);
    }
    auto get_fluent_facts() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    {
        return ygg::make_view(this->get_data().fluent_facts, *this->m_context);
    }
    auto get_ground_actions() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    {
        return ygg::make_view(this->get_data().ground_actions, *this->m_context);
    }
    auto get_ground_axioms() const noexcept
        requires std::same_as<Kind, ::tyr::GroundTag>
    {
        return ygg::make_view(this->get_data().ground_axioms, *this->m_context);
    }
};

}

#endif
