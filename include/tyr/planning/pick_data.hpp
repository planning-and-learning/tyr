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

#ifndef TYR_PLANNING_PICK_DATA_HPP_
#define TYR_PLANNING_PICK_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <type_traits>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>

namespace urd
{

/// An outcome of a choice of `action` that nature may pick, for one binding of the choice's variables (lifted: the action
/// binding followed by the objects of the variables of the choice and of the choices it is nested in; ground: none).
template<::tyr::TaskKind Kind>
struct data<::tyr::planning::Pick<Kind>>
{
    urd::key<::tyr::planning::Pick<Kind>> index;
    urd::index<::tyr::formalism::Action<Kind>> action;
    urd::index<::tyr::formalism::Outcome<Kind>> outcome;
    urd::flat_vector<urd::index<::tyr::formalism::Object>> binding;

    data() = default;

    data(urd::index<::tyr::formalism::Action<Kind>> action_, urd::index<::tyr::formalism::Outcome<Kind>> outcome_, urd::flat_vector<urd::index<::tyr::formalism::Object>> binding_) : index(), action(std::move(action_)), outcome(std::move(outcome_)), binding(std::move(binding_)) {}

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"action", &data::action>, urd::field<"outcome", &data::outcome>, urd::field<"binding", &data::binding>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::members_of(fields()); }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::action, &data::outcome, &data::binding> {}; }

    [[nodiscard]] urd::key<::tyr::planning::Pick<Kind>> key() const noexcept { return urd::key<::tyr::planning::Pick<Kind>>(index); }
};

}

#endif
