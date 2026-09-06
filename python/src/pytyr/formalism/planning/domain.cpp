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

void bind_domain(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<Domain>>(m, "DomainIndex");

    {
        using V = ygg::Data<Domain>;

        auto cls = nb::class_<V>(m, "DomainData")  //
                       .def(nb::init<const std::string&,
                                     const PredicateViewList<StaticTag>&,
                                     const PredicateViewList<FluentTag>&,
                                     const PredicateViewList<DerivedTag>&,
                                     const FunctionViewList<StaticTag>&,
                                     const FunctionViewList<FluentTag>&,
                                     const std::optional<FunctionView<AuxiliaryTag>>&,
                                     const ObjectViewList&,
                                     const ActionViewList<::tyr::LiftedTag>&,
                                     const AxiomViewList<::tyr::LiftedTag>&>(),
                            "name"_a,
                            "static_predicates"_a,
                            "fluent_predicates"_a,
                            "derived_predicates"_a,
                            "static_functions"_a,
                            "fluent_functions"_a,
                            "auxiliary_function"_a,
                            "constants"_a,
                            "actions"_a,
                            "axioms"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = DomainView;

        auto cls = nb::class_<V>(m, "Domain")  //
                       .def("get_index", &V::get_index)
                       .def("get_name", &V::get_name)
                       .def("get_static_predicates", &V::get_predicates<StaticTag>)
                       .def("get_fluent_predicates", &V::get_predicates<FluentTag>)
                       .def("get_derived_predicates", &V::get_predicates<DerivedTag>)
                       .def("get_static_functions", &V::get_functions<StaticTag>)
                       .def("get_fluent_functions", &V::get_functions<FluentTag>)
                       .def("get_auxiliary_function", &V::get_auxiliary_function)
                       .def("get_constants", &V::get_constants)
                       .def("get_actions", &V::get_actions)
                       .def("get_axioms", &V::get_axioms);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Domain>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::planning
