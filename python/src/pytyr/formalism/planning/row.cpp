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

#include "binding_utils.hpp"
#include "bindings.hpp"
#include "pytyr/module.hpp"

#include <tyr/formalism/declarations.hpp>
#include <yggdrasil/formalism/row_data.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::formalism::planning
{

namespace
{
template<typename Relation>
void bind_row_view(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_view<ygg::RowView<Relation, Repository>>(m);
    ygg::bind_insert<ygg::Row<Relation>>(repository);
}
}  // namespace

void bind_row(nb::module_& m, RepositoryBinding& repository)
{
    // The rows of its own relations; those of predicates and functions are bound with the formalism's records.
    urd::python::bind_value<ygg::RowIndex<Predicate<DerivedTag>>>(m);
    urd::python::bind_data<ygg::Row<Predicate<DerivedTag>>>(m);
    urd::python::bind_value<ygg::RowIndex<Action<LiftedTag>>>(m);
    urd::python::bind_data<ygg::Row<Action<LiftedTag>>>(m);
    urd::python::bind_value<ygg::RowIndex<Axiom<LiftedTag>>>(m);
    urd::python::bind_data<ygg::Row<Axiom<LiftedTag>>>(m);

    bind_row_view<Predicate<DerivedTag>>(m, repository);
    bind_row_view<Action<LiftedTag>>(m, repository);
    bind_row_view<Axiom<LiftedTag>>(m, repository);
    bind_row_view<Predicate<StaticTag>>(m, repository);
    bind_row_view<Predicate<FluentTag>>(m, repository);
    bind_row_view<Function<StaticTag>>(m, repository);
    bind_row_view<Function<FluentTag>>(m, repository);
    bind_row_view<Function<AuxiliaryTag>>(m, repository);
}

}  // namespace tyr::formalism::planning
