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

#ifndef TYR_FORMALISM_PLANNING_CONJUNCTIVE_CONDITION_VIEW_HPP_
#define TYR_FORMALISM_PLANNING_CONJUNCTIVE_CONDITION_VIEW_HPP_

#include "tyr/formalism/planning/boolean_operator_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/literal_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <ranges>
#include <utility>
#include <yggdrasil/containers/vector.hpp>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/formalism/detail/view.hpp>

namespace ygg
{
template<::tyr::TaskKind T, ygg::formalism::SymbolContextFor<::tyr::formalism::planning::ConjunctiveCondition<T>> C>
class View<ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<T>>, C> :
    public ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<T>>, C>
{
public:
    View(ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<T>> handle, const C& context) noexcept :
        ygg::formalism::detail::View<ygg::Index<::tyr::formalism::planning::ConjunctiveCondition<T>>, C>(handle, context)
    {
    }

    auto get_variables() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return ygg::make_view(this->get_data().variables, *this->m_context);
    }
    template<::tyr::formalism::FactKind F>
    auto get_literals() const noexcept
    {
        return ygg::make_view(this->get_data().template get_literals<F>(), *this->m_context);
    }
    auto get_numeric_constraints() const noexcept(std::same_as<T, ::tyr::GroundTag>)
    {
        return ygg::make_view(this->get_data().numeric_constraints, *this->m_context);
    }
    auto get_arity() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return this->get_data().variables.size();
    }

    template<::tyr::formalism::PolarityKind F>
    auto get_facts() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    {
        return ygg::make_view(this->get_data().template get_facts<F>(), *this->m_context);
    }

    /// Projects represented atoms of the requested polarity, borrowing the repository.
    template<::tyr::formalism::FactKind F>
    auto get_atoms_view(bool polarity) const
    {
        if constexpr (std::same_as<T, ::tyr::GroundTag> && std::same_as<F, ::tyr::formalism::FluentTag>)
        {
            auto facts = polarity ? get_facts<::tyr::formalism::PositiveTag>() : get_facts<::tyr::formalism::NegativeTag>();
            return std::move(facts) | std::views::filter([](auto fact) { return fact.has_value(); })
                   | std::views::transform([](auto fact) { return *fact.get_atom(); });
        }
        else
        {
            return get_literals<F>() | std::views::filter([polarity](auto literal) { return literal.get_polarity() == polarity; })
                   | std::views::transform([](auto literal) { return literal.get_atom(); });
        }
    }
};

}

#endif
