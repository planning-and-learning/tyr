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

void bind_fdr_task(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<FDRTask>>(m, "GroundTaskIndex");

    {
        using V = ygg::Data<FDRTask>;

        auto cls = nb::class_<V>(m, "GroundTaskData")  //

                       .def(nb::init<const std::string&,
                                     DomainView,
                                     const PredicateViewList<DerivedTag>&,
                                     const ObjectViewList&,
                                     const AtomViewList<::tyr::GroundTag, StaticTag>&,
                                     const AtomViewList<::tyr::GroundTag, FluentTag>&,
                                     const AtomViewList<::tyr::GroundTag, DerivedTag>&,
                                     const FunctionTermValueViewList<::tyr::GroundTag, StaticTag>&,
                                     const FunctionTermValueViewList<::tyr::GroundTag, FluentTag>&,
                                     const std::optional<FunctionTermValueView<::tyr::GroundTag, AuxiliaryTag>>&,
                                     const std::optional<MetricView>&,
                                     const AxiomViewList<::tyr::LiftedTag>&,
                                     const FDRVariableViewList<FluentTag>&,
                                     const FDRFactViewList<FluentTag>&,
                                     ConjunctiveConditionView<::tyr::GroundTag>,
                                     const ActionViewList<::tyr::GroundTag>&,
                                     const AxiomViewList<::tyr::GroundTag>&>(),
                            "name"_a,
                            "domain"_a,
                            "derived_predicates"_a,
                            "objects"_a,
                            "static_atoms"_a,
                            "fluent_atoms"_a,
                            "derived_atoms"_a,
                            "static_fterm_values"_a,
                            "fluent_fterm_values"_a,
                            "auxiliary_fterm_value"_a,
                            "metric"_a,
                            "axioms"_a,
                            "fluent_variables"_a,
                            "fluent_facts"_a,
                            "goal"_a,
                            "ground_actions"_a,
                            "ground_axioms"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = FDRTaskView;

        auto cls = nb::class_<V>(m, "GroundTask")  //
                       .def("get_index", &V::get_index)
                       .def("get_name", &V::get_name)
                       .def("get_domain", &V::get_domain, nb::keep_alive<0, 1>())
                       .def("get_derived_predicates", &V::get_derived_predicates)
                       .def("get_objects", &V::get_objects)
                       .def("get_static_atoms", &V::get_atoms<StaticTag>)
                       .def("get_fluent_atoms", &V::get_atoms<FluentTag>)
                       .def("get_derived_atoms", &V::get_atoms<DerivedTag>)
                       .def("get_static_fterm_values", &V::get_fterm_values<StaticTag>)
                       .def("get_fluent_fterm_values", &V::get_fterm_values<FluentTag>)
                       .def("get_auxiliary_fterm_value", &V::get_auxiliary_fterm_value)
                       .def("get_goal", &V::get_goal, nb::keep_alive<0, 1>())
                       .def("get_metric", &V::get_metric)
                       .def("get_axioms", &V::get_axioms)
                       .def("get_fluent_variables", &V::get_fluent_variables)
                       .def("get_fluent_facts", &V::get_fluent_facts)
                       .def("get_ground_actions", &V::get_ground_actions)
                       .def("get_ground_axioms", &V::get_ground_axioms);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<FDRTask>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::planning
