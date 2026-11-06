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

#ifndef PYTHON_SRC_PYTYR_FORMALISM_PLANNING_BINDING_UTILS_HPP_
#define PYTHON_SRC_PYTYR_FORMALISM_PLANNING_BINDING_UTILS_HPP_

#include "../binding_utils.hpp"
#include "bindings.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/planning/formatter.hpp>
#include <tyr/formalism/planning/interning.hpp>
#include <tyr/formalism/planning/repository.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <urd/python/owner.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::formalism::planning
{


template<typename T>
void bind_insert(RepositoryBinding& repository)
{
    repository.def("insert", [](Repository& self, urd::data<T>& data) { return insert(self, data); }, nb::arg("data"));
}

template<typename T>
urd::borrowed_view<T, Repository> create_data(Repository& repository, const urd::data<T>& data)
{ return urd::resolve(data, repository); }

}  // namespace tyr::formalism::planning

#endif
