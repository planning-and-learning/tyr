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

void bind_ground_axiom(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<GroundAxiom>>(m, "GroundAxiomIndex");

    {
        using V = ygg::Data<GroundAxiom>;

        auto cls = nb::class_<V>(m, "GroundAxiomData")  //
                       .def(nb::init<AxiomBindingView, GroundConjunctiveConditionView, GroundAtomView<DerivedTag>>(), "binding"_a, "body"_a, "head"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = GroundAxiomView;

        auto cls = nb::class_<V>(m, "GroundAxiom")  //
                       .def("get_index", &V::get_index)
                       .def("get_axiom", &V::get_axiom, nb::keep_alive<0, 1>())
                       .def("get_row", &V::get_row, nb::keep_alive<0, 1>())
                       .def("get_objects", &V::get_objects)
                       .def("get_key", &V::get_key)
                       .def("get_body", &V::get_body, nb::keep_alive<0, 1>())
                       .def("get_head", &V::get_head, nb::keep_alive<0, 1>());
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<GroundAxiom>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::planning
