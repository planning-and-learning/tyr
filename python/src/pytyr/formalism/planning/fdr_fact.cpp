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

#include <tyr/formalism/planning/fdr_fact_data.hpp>
#include <tyr/formalism/planning/fdr_fact_view.hpp>

namespace tyr::formalism::planning
{

namespace
{
template<FactKind T>
void bind_fdr_fact_data(nb::module_& m, const char* name)
{
    using V = ygg::Data<FDRFact<T>>;
    auto cls = nb::class_<V>(m, name).def(nb::init<FDRVariableView<T>, FDRValue>(), "variable"_a, "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_fdr_fact_view(nb::module_& m, const char* name)
{
    using V = FDRFactView<T>;
    auto cls = nb::class_<V>(m, name)
                   .def("get_variable", &V::get_variable, nb::keep_alive<0, 1>())
                   .def("get_value", &V::get_value)
                   .def("has_value", &V::has_value)
                   .def("get_atom", &V::get_atom, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_fdr_fact(nb::module_& m, RepositoryBinding&)
{
    bind_fdr_fact_data<FluentTag>(m, "FluentFDRFactData");
    bind_fdr_fact_view<FluentTag>(m, "FluentFDRFact");
}

}  // namespace tyr::formalism::planning
