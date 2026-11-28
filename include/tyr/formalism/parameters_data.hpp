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

#ifndef TYR_FORMALISM_PARAMETERS_DATA_HPP_
#define TYR_FORMALISM_PARAMETERS_DATA_HPP_

#include "tyr/formalism/declarations.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>

namespace urd
{

/// The variables of a scope, by parameter index: those of an action, an axiom or a rule, a conditional effect's after its
/// action's. The lifted atoms and function terms of a scope share it, and name each parameter by it.
template<>
struct data<::tyr::formalism::Parameters>
{
    urd::key<::tyr::formalism::Parameters> index;
    urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables;

    data() = default;

    data(urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_) : index(), variables(std::move(variables_)) {}

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) noexcept = default;
    data& operator=(data&& other) noexcept = default;
    ~data() = default;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"variables", &data::variables>> {}; }

    static constexpr auto cista_members() noexcept { return urd::cista_members_of<data>(); }

    [[nodiscard]] urd::key<::tyr::formalism::Parameters> key() const noexcept { return urd::key<::tyr::formalism::Parameters>(index); }
};

}

#endif
