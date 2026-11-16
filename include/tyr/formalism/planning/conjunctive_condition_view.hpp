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

#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <ranges>
#include <urd/elements/flat.hpp>
#include <utility>

namespace urd
{
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::planning::ConjunctiveCondition<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_variables() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::variables>(); }
    auto get_static_literals() const noexcept { return self().template member<&urd::record_of<V>::static_literals>(); }
    auto get_fluent_literals() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::fluent_literals>(); }
    auto get_derived_literals() const noexcept { return self().template member<&urd::record_of<V>::derived_literals>(); }
    auto get_positive_facts() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::positive_facts>(); }
    auto get_negative_facts() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::negative_facts>(); }
    auto get_numeric_constraints() const noexcept(std::same_as<T, ::tyr::GroundTag>)
    { return self().template member<&urd::record_of<V>::numeric_constraints>(); }
    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind F>
    auto get_literals() const noexcept
    { return self().template member<::tyr::formalism::planning::literals_of<F, T>>(); }
    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::PolarityKind F>
    auto get_facts() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return self().template member<::tyr::formalism::planning::condition_facts_of<F>>(); }
    auto get_arity() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return get_variables().size(); }
    /// Whether the condition has no literals and no numeric constraints, i.e., holds in every state.
    [[nodiscard]] bool empty() const
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return get_literals<::tyr::formalism::StaticTag>().empty() && get_literals<::tyr::formalism::FluentTag>().empty()
               && get_literals<::tyr::formalism::DerivedTag>().empty() && get_numeric_constraints().empty();
    }

    /// Projects represented atoms of the requested polarity, borrowing the repository.
    template<::tyr::formalism::FactKind F>
    auto get_atoms(bool polarity) const
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

    template<::tyr::formalism::FactKind F, typename P>
    auto get_atoms(bool polarity, P predicate) const
    {
        return get_atoms<F>(polarity) | std::views::filter([predicate](auto atom) { return atom.get_predicate() == predicate; });
    }
};
}  // namespace urd

#endif
