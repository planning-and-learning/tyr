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

namespace tyr::formalism::planning
{

void bind_axiom(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<Axiom>>(m, "AxiomIndex");

    {
        using V = ygg::Data<Axiom>;

        auto cls = nb::class_<V>(m, "AxiomData")  //
                       .def(nb::init<const VariableViewList&, ConjunctiveConditionView, AtomView<DerivedTag>>(), "variables"_a, "body"_a, "head"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = AxiomView;

        auto cls = nb::class_<V>(m, "Axiom")  //
                       .def("get_index", &V::get_index)
                       .def("get_arity", &V::get_arity)
                       .def("get_variables", &V::get_variables)
                       .def("get_body", &V::get_body, nb::keep_alive<0, 1>())
                       .def("get_head", &V::get_head, nb::keep_alive<0, 1>());
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Axiom>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::planning
