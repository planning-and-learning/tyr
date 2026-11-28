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

#ifndef TYR_FORMALISM_PLANNING_TEMPORAL_PART_DATA_HPP_
#define TYR_FORMALISM_PLANNING_TEMPORAL_PART_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>

namespace urd
{

/// What makes an action durative (PDDL 2.1): the bounds of its duration (over the action's variables: lower and upper
/// bounds evaluated when it starts, and those a duration constraint timed at its end evaluates then; = gives both), the
/// condition that holds while it runs (over all), the condition and the effects at its end. The action's own condition,
/// effects and choices are those at its start.
template<::tyr::TaskKind T>
struct data<::tyr::formalism::TemporalPart<T>>
{
    urd::key<::tyr::formalism::TemporalPart<T>> index;
    urd::flat_vector<::tyr::formalism::FunctionExpression<T>> min_durations;
    urd::flat_vector<::tyr::formalism::FunctionExpression<T>> max_durations;
    urd::flat_vector<::tyr::formalism::FunctionExpression<T>> end_min_durations;
    urd::flat_vector<::tyr::formalism::FunctionExpression<T>> end_max_durations;
    urd::index<::tyr::formalism::ConjunctiveCondition<T>> over_all;
    urd::index<::tyr::formalism::ConjunctiveCondition<T>> end_condition;
    urd::flat_vector<urd::index<::tyr::formalism::ConditionalEffect<T>>> end_effects;
    urd::flat_vector<urd::index<::tyr::formalism::ChoiceEffect<T>>> end_choices;

    data() = default;

    data(urd::flat_vector<::tyr::formalism::FunctionExpression<T>> min_durations_,
         urd::flat_vector<::tyr::formalism::FunctionExpression<T>> max_durations_,
         urd::flat_vector<::tyr::formalism::FunctionExpression<T>> end_min_durations_,
         urd::flat_vector<::tyr::formalism::FunctionExpression<T>> end_max_durations_,
         urd::index<::tyr::formalism::ConjunctiveCondition<T>> over_all_,
         urd::index<::tyr::formalism::ConjunctiveCondition<T>> end_condition_,
         urd::flat_vector<urd::index<::tyr::formalism::ConditionalEffect<T>>> end_effects_,
         urd::flat_vector<urd::index<::tyr::formalism::ChoiceEffect<T>>> end_choices_) :
        index(),
        min_durations(std::move(min_durations_)),
        max_durations(std::move(max_durations_)),
        end_min_durations(std::move(end_min_durations_)),
        end_max_durations(std::move(end_max_durations_)),
        over_all(over_all_),
        end_condition(end_condition_),
        end_effects(std::move(end_effects_)),
        end_choices(std::move(end_choices_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) noexcept = default;
    data& operator=(data&& other) noexcept = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"min_durations", &data::min_durations>,
                           urd::field<"max_durations", &data::max_durations>,
                           urd::field<"end_min_durations", &data::end_min_durations>,
                           urd::field<"end_max_durations", &data::end_max_durations>,
                           urd::field<"over_all", &data::over_all>,
                           urd::field<"end_condition", &data::end_condition>,
                           urd::field<"end_effects", &data::end_effects>,
                           urd::field<"end_choices", &data::end_choices>> {};
    }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::TemporalPart<T>> key() const noexcept { return urd::key<::tyr::formalism::TemporalPart<T>>(index); }
};

}  // namespace urd

#endif
