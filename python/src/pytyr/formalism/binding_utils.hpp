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

#ifndef PYTHON_SRC_PYTYR_FORMALISM_BINDING_UTILS_HPP_
#define PYTHON_SRC_PYTYR_FORMALISM_BINDING_UTILS_HPP_

#include "bindings.hpp"

#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/datalog/declarations.hpp>
#include <tyr/formalism/declarations.hpp>
#include <yggdrasil/formalism/binding_data.hpp>
#include <tyr/formalism/function_data.hpp>
#include <tyr/formalism/object_data.hpp>
#include <tyr/formalism/parameter_index.hpp>
#include <tyr/formalism/predicate_data.hpp>
#include <tyr/formalism/variable_data.hpp>
#include <tyr/formalism/formatter.hpp>
#include <tyr/formalism/binding_index.hpp>
#include <tyr/formalism/planning/declarations.hpp>
#include <yggdrasil/formalism/binding_view.hpp>
#include <tyr/formalism/function_view.hpp>
#include <tyr/formalism/object_view.hpp>
#include <tyr/formalism/predicate_view.hpp>
#include <tyr/formalism/variable_view.hpp>
#include <yggdrasil/python/bindings.hpp>
#include "pytyr/module.hpp"
#include <urd/elements/flat.hpp>

namespace tyr::formalism
{
template<typename V>
using VariantT = typename V::Variant;
}

#endif
