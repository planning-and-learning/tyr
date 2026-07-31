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

void bind_program(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<Program>>(m, "ProgramIndex");

    {
        using V = ygg::Data<Program>;
        auto cls = nb::class_<V>(m, "ProgramData")  //
                       .def(nb::init<const PredicateViewList<StaticTag>&,
                                     const PredicateViewList<FluentTag>&,
                                     const FunctionViewList<StaticTag>&,
                                     const FunctionViewList<FluentTag>&,
                                     const ObjectViewList&,
                                     const GroundAtomViewList<StaticTag>&,
                                     const GroundAtomViewList<FluentTag>&,
                                     const GroundFunctionTermValueViewList<StaticTag>&,
                                     const GroundFunctionTermValueViewList<FluentTag>&,
                                     const std::optional<GroundConjunctiveConditionView>&,
                                     const std::optional<MetricView>&,
                                     const RuleViewList<PredicateTag>&,
                                     const RuleViewList<FunctionTag>&>(),
                            "static_predicates"_a,
                            "fluent_predicates"_a,
                            "static_functions"_a,
                            "fluent_functions"_a,
                            "objects"_a,
                            "static_atoms"_a,
                            "fluent_atoms"_a,
                            "static_fterm_values"_a,
                            "fluent_fterm_values"_a,
                            "goal"_a,
                            "metric"_a,
                            "rules"_a,
                            "function_rules"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = ProgramView<LiftedTag>;
        auto cls = nb::class_<V>(m, "Program")  //
                       .def("get_index", &V::get_index)
                       .def("get_static_predicates", &V::template get_predicates<StaticTag>)
                       .def("get_fluent_predicates", &V::template get_predicates<FluentTag>)
                       .def("get_static_functions", &V::template get_functions<StaticTag>)
                       .def("get_fluent_functions", &V::template get_functions<FluentTag>)
                       .def("get_objects", &V::get_objects)
                       .def("get_static_atoms", &V::template get_atoms<StaticTag>)
                       .def("get_fluent_atoms", &V::template get_atoms<FluentTag>)
                       .def("get_static_fterm_values", &V::template get_fterm_values<StaticTag>)
                       .def("get_fluent_fterm_values", &V::template get_fterm_values<FluentTag>)
                       .def("get_goal", &V::get_goal, nb::keep_alive<0, 1>())
                       .def("get_metric", &V::get_metric);
        cls.def("get_rules", &V::template get_rules<PredicateTag>);
        cls.def("get_function_rules", &V::template get_rules<FunctionTag>);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Program>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::datalog
