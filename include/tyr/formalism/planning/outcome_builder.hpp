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

#ifndef TYR_FORMALISM_PLANNING_OUTCOME_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_OUTCOME_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/conditional_effect_builder.hpp"
#include "tyr/formalism/planning/outcome_data.hpp"
#include "tyr/formalism/planning/outcome_view.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

/// The builder of Outcome: its record's members, each in its built form (urd::built_t).
template<::tyr::TaskKind T, typename C>
struct builder<::tyr::formalism::Outcome<T>, C>
{
    urd::key<::tyr::formalism::Outcome<T>> index;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::ConditionalEffect<T>>>, C> effects;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::ChoiceEffect<T>>>, C> choices;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"effects", &builder::effects>, urd::field<"choices", &builder::choices>> {}; }
};

}  // namespace urd

#endif
