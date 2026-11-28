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

#ifndef TYR_FORMALISM_PLANNING_OUTCOME_DATA_HPP_
#define TYR_FORMALISM_PLANNING_OUTCOME_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>

namespace urd
{

/// One outcome of a choice of nature: the flat conditional effects that happen when nature picks it, and the choices
/// nested in it (a probabilistic effect in a oneof outcome, or the reverse; neither: nothing happens). Their variables
/// and conditions extend those of the choice effect that holds the outcome.
template<::tyr::TaskKind T>
struct data<::tyr::formalism::Outcome<T>>
{
    urd::key<::tyr::formalism::Outcome<T>> index;
    urd::flat_vector<urd::index<::tyr::formalism::ConditionalEffect<T>>> effects;
    urd::flat_vector<urd::index<::tyr::formalism::ChoiceEffect<T>>> choices;

    data() = default;

    data(urd::flat_vector<urd::index<::tyr::formalism::ConditionalEffect<T>>> effects_,
         urd::flat_vector<urd::index<::tyr::formalism::ChoiceEffect<T>>> choices_) :
        index(),
        effects(std::move(effects_)),
        choices(std::move(choices_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) noexcept = default;
    data& operator=(data&& other) noexcept = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"effects", &data::effects>, urd::field<"choices", &data::choices>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::Outcome<T>> key() const noexcept { return urd::key<::tyr::formalism::Outcome<T>>(index); }
};

}  // namespace urd

#endif
