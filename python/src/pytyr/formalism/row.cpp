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

#include "bindings.hpp"

#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/formalism/declarations.hpp>
#include <tyr/formalism/formatter.hpp>
#include <yggdrasil/formalism/declarations.hpp>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::formalism
{
namespace
{
/// A row of Relation: its index (relation and row) and its record.
template<typename Relation>
void bind_row_of(nb::module_& m)
{
    urd::python::bind_index<ygg::RowIndex<Relation>>(m);
    urd::python::bind_data<ygg::Row<Relation>>(m);
}
}  // namespace

void bind_row(nb::module_& m)
{
    bind_row_of<Predicate<StaticTag>>(m);
    bind_row_of<Predicate<FluentTag>>(m);
    bind_row_of<Predicate<DerivedTag>>(m);
    bind_row_of<Function<StaticTag>>(m);
    bind_row_of<Function<FluentTag>>(m);
    bind_row_of<Function<AuxiliaryTag>>(m);
}

}  // namespace tyr::formalism
