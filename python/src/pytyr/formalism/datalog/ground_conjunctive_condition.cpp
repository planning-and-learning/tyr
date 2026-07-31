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

namespace tyr::formalism::datalog
{

void bind_ground_conjunctive_condition(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<GroundConjunctiveCondition>>(m, "GroundConjunctiveConditionIndex");

    {
        using V = ygg::Data<GroundConjunctiveCondition>;
        auto cls = nb::class_<V>(m, "GroundConjunctiveConditionData")  //
                       .def(nb::init<const GroundLiteralViewList<StaticTag>&, const GroundLiteralViewList<FluentTag>&, const GroundBooleanOperatorViewList&>(),
                            "static_literals"_a,
                            "fluent_literals"_a,
                            "numeric_constraints"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = GroundConjunctiveConditionView;
        auto cls = nb::class_<V>(m, "GroundConjunctiveCondition")  //
                       .def("get_index", &V::get_index)
                       .def("get_static_literals", &V::get_literals<StaticTag>)
                       .def("get_fluent_literals", &V::get_literals<FluentTag>)
                       .def("get_numeric_constraints", &V::get_numeric_constraints);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<GroundConjunctiveCondition>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::datalog
