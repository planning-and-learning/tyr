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

#ifndef TYR_FORMALISM_PLANNING_CHOICE_EFFECT_DATA_HPP_
#define TYR_FORMALISM_PLANNING_CHOICE_EFFECT_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace urd
{

/// A choice of nature (a oneof, or a probabilistic effect) under its forall variables and when condition: when it fires,
/// for each binding of its variables, nature picks one of its outcomes. Its probabilities, one per outcome, are given for a
/// probabilistic effect and empty for a oneof (the task's environment says which it is).
template<>
struct data<::tyr::formalism::ChoiceEffect<::tyr::LiftedTag>>
{
    urd::key<::tyr::formalism::ChoiceEffect<::tyr::LiftedTag>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables;
    urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>> condition;
    urd::flat_vector<urd::index<::tyr::formalism::Outcome<::tyr::LiftedTag>>> outcomes;
    urd::flat_vector<ygg::float_t> probabilities;

    data() = default;

    data(urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_,
         urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::LiftedTag>> condition_,
         urd::flat_vector<urd::index<::tyr::formalism::Outcome<::tyr::LiftedTag>>> outcomes_,
         urd::flat_vector<ygg::float_t> probabilities_) :
        index(),
        variables(std::move(variables_)),
        condition(condition_),
        outcomes(std::move(outcomes_)),
        probabilities(std::move(probabilities_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) noexcept = default;
    data& operator=(data&& other) noexcept = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"variables", &data::variables>,
                           urd::field<"condition", &data::condition>,
                           urd::field<"outcomes", &data::outcomes>,
                           urd::field<"probabilities", &data::probabilities>> {};
    }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::ChoiceEffect<::tyr::LiftedTag>> key() const noexcept
    { return urd::key<::tyr::formalism::ChoiceEffect<::tyr::LiftedTag>>(index); }
};

/// A ground choice of nature: its condition and outcomes for one binding of its variables.
template<>
struct data<::tyr::formalism::ChoiceEffect<::tyr::GroundTag>>
{
    urd::key<::tyr::formalism::ChoiceEffect<::tyr::GroundTag>> index;
    urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>> condition;
    urd::flat_vector<urd::index<::tyr::formalism::Outcome<::tyr::GroundTag>>> outcomes;
    urd::flat_vector<ygg::float_t> probabilities;

    data() = default;

    data(urd::index<::tyr::formalism::ConjunctiveCondition<::tyr::GroundTag>> condition_,
         urd::flat_vector<urd::index<::tyr::formalism::Outcome<::tyr::GroundTag>>> outcomes_,
         urd::flat_vector<ygg::float_t> probabilities_) :
        index(),
        condition(condition_),
        outcomes(std::move(outcomes_)),
        probabilities(std::move(probabilities_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) noexcept = default;
    data& operator=(data&& other) noexcept = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::
            Fields<urd::field<"condition", &data::condition>, urd::field<"outcomes", &data::outcomes>, urd::field<"probabilities", &data::probabilities>> {};
    }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::ChoiceEffect<::tyr::GroundTag>> key() const noexcept
    { return urd::key<::tyr::formalism::ChoiceEffect<::tyr::GroundTag>>(index); }
};

}  // namespace urd

#endif
