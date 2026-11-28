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

#ifndef TYR_FORMALISM_OBJECT_BUILDER_HPP_
#define TYR_FORMALISM_OBJECT_BUILDER_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"

#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <variant>

namespace urd
{

/// The builder of Object: its record's members, each in its built form (urd::built_t).
template<typename C>
struct builder<::tyr::formalism::Object, C>
{
    urd::key<::tyr::formalism::Object> index;
    urd::built_t<urd::flat_string, C> name;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"name", &builder::name>> {}; }

};

}  // namespace urd

namespace tyr::formalism
{

/// A term as a builder holds it (Term built): an object, as a child (built, or interned), or a parameter.
template<typename C>
using BuiltTerm = std::variant<urd::child<Object, C>, ParameterIndex>;

}  // namespace tyr::formalism

#endif
