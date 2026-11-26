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
#include "pytyr/formalism/conversions.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/repository.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <urd/python/bindings.hpp>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::formalism
{

namespace
{
template<FactKind T>
void bind_predicate_view(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_view<PredicateView<T>>(m);
    python::bind_insert<Predicate<T>>(repository);
    urd::python::bind_builder<Predicate<T>, Repository>(m);
}
}  // namespace

void bind_predicate(nb::module_& m, RepositoryBinding& repository)
{
    bind_predicate_view<StaticTag>(m, repository);
    bind_predicate_view<FluentTag>(m, repository);
    bind_predicate_view<DerivedTag>(m, repository);
}

}  // namespace tyr::formalism
