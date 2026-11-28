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

#ifndef TYR_FORMALISM_PLANNING_TEMPORAL_PART_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_TEMPORAL_PART_BUILDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/temporal_part_data.hpp"
#include "tyr/formalism/planning/temporal_part_view.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

/// The builder of TemporalPart: its record's members, each in its built form (urd::built_t).
template<::tyr::TaskKind T, typename C>
struct builder<::tyr::formalism::TemporalPart<T>, C>
{
    urd::key<::tyr::formalism::TemporalPart<T>> index;
    urd::built_t<urd::flat_vector<::tyr::formalism::FunctionExpression<T>>, C> min_durations;
    urd::built_t<urd::flat_vector<::tyr::formalism::FunctionExpression<T>>, C> max_durations;
    urd::built_t<urd::flat_vector<::tyr::formalism::FunctionExpression<T>>, C> end_min_durations;
    urd::built_t<urd::flat_vector<::tyr::formalism::FunctionExpression<T>>, C> end_max_durations;
    urd::built_t<urd::index<::tyr::formalism::ConjunctiveCondition<T>>, C> over_all;
    urd::built_t<urd::index<::tyr::formalism::ConjunctiveCondition<T>>, C> end_condition;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::ConditionalEffect<T>>>, C> end_effects;
    urd::built_t<urd::flat_vector<urd::index<::tyr::formalism::ChoiceEffect<T>>>, C> end_choices;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"min_durations", &builder::min_durations>,
                           urd::field<"max_durations", &builder::max_durations>,
                           urd::field<"end_min_durations", &builder::end_min_durations>,
                           urd::field<"end_max_durations", &builder::end_max_durations>,
                           urd::field<"over_all", &builder::over_all>,
                           urd::field<"end_condition", &builder::end_condition>,
                           urd::field<"end_effects", &builder::end_effects>,
                           urd::field<"end_choices", &builder::end_choices>> {};
    }
};

}  // namespace urd

#endif
