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

#include <tyr/formalism/planning/fdr_variable_data.hpp>
#include <tyr/formalism/planning/fdr_variable_index.hpp>
#include <tyr/formalism/planning/fdr_variable_view.hpp>

namespace tyr::formalism::planning
{

namespace
{
template<FactKind T>
void bind_fdr_variable_data(nb::module_& m, const char* name)
{
    using V = ygg::Data<FDRVariable<T>>;
    auto cls = nb::class_<V>(m, name).def(nb::init<const AtomViewList<GroundTag, T>>(), "atoms"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_fdr_variable_view(nb::module_& m, const char* name)
{
    using V = FDRVariableView<T>;
    auto cls = nb::class_<V>(m, name).def("get_index", &V::get_index).def("get_domain_size", &V::get_domain_size).def("get_atoms", &V::get_atoms);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_fdr_variable(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<FDRVariable<FluentTag>>>(m, "FluentFDRVariableIndex");

    bind_fdr_variable_data<FluentTag>(m, "FluentFDRVariableData");

    bind_fdr_variable_view<FluentTag>(m, "FluentFDRVariable");

    repository.def("get_or_create", &get_or_create_data<FDRVariable<FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::planning
