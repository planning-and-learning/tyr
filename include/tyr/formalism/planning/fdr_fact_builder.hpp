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

#ifndef TYR_FORMALISM_PLANNING_FDR_FACT_BUILDER_HPP_
#define TYR_FORMALISM_PLANNING_FDR_FACT_BUILDER_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/fdr_variable_builder.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/index.hpp>

namespace urd
{

/// The builder of FDRFact: its record's members, each in its built form (urd::built_t).
template<::tyr::formalism::FactKind T, typename C>
struct builder<::tyr::formalism::FDRFact<T>, C>
{
    urd::built_t<urd::index<::tyr::formalism::FDRVariable<T>>, C> variable;
    urd::built_t<::tyr::formalism::FDRValue, C> value;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"variable", &builder::variable>, urd::field<"value", &builder::value>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::variable, &builder::value> {}; }
};

}  // namespace urd

#endif
