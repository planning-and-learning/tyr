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

#ifndef PYTHON_SRC_PYTYR_FORMALISM_DATALOG_BINDING_UTILS_HPP_
#define PYTHON_SRC_PYTYR_FORMALISM_DATALOG_BINDING_UTILS_HPP_

#include "bindings.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/datalog/formatter.hpp>
#include <tyr/formalism/datalog/repository.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::formalism::datalog
{

template<typename T>
using Data = ygg::Data<T>;

template<typename T>
ygg::View<ygg::Index<T>, Repository> get_or_create_data(Repository& repository, Data<T>& data)
{
    canonicalize(data);
    return repository.template get_or_create<T>(data).first;
}

template<typename T>
ygg::View<ygg::Index<RelationBinding<T>>, Repository> get_or_create_relation_data(Repository& repository, const Data<RelationBinding<T>>& data)
{
    return repository.template get_or_create<T>(data).first;
}

template<typename T>
ygg::View<ygg::Data<T>, Repository> create_data(Repository& repository, const Data<T>& data)
{
    return make_view(data, repository);
}

}  // namespace tyr::formalism::datalog

#endif
