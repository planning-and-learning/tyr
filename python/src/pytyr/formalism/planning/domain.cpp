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
#include "pytyr/module.hpp"
#include "bindings.hpp"

namespace tyr::formalism::planning
{

void bind_domain(nb::module_& m, RepositoryBinding& repository)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<Domain>>(m, "DomainIndex");

    {
        using V = urd::data<Domain>;

        auto cls = nb::class_<V>(m, "DomainData")  //
                       .def(urd::python::init_from_views<const std::string&,
                                     const PredicateViewList<StaticTag>&,
                                     const PredicateViewList<FluentTag>&,
                                     const PredicateViewList<DerivedTag>&,
                                     const FunctionViewList<StaticTag>&,
                                     const FunctionViewList<FluentTag>&,
                                     const std::optional<FunctionView<AuxiliaryTag>>&,
                                     const ObjectViewList&,
                                     const ActionViewList<LiftedTag>&,
                                     const AxiomViewList<LiftedTag>&>(),
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
        cls.def(nb::init<urd::flat_string,
                         urd::flat_vector<urd::index<Predicate<StaticTag>>>,
                         urd::flat_vector<urd::index<Predicate<FluentTag>>>,
                         urd::flat_vector<urd::index<Predicate<DerivedTag>>>,
                         urd::flat_vector<urd::index<Function<StaticTag>>>,
                         urd::flat_vector<urd::index<Function<FluentTag>>>,
                         urd::flat_optional<urd::index<Function<AuxiliaryTag>>>,
                         urd::flat_vector<urd::index<Object>>,
                         urd::flat_vector<urd::index<Action<LiftedTag>>>,
                         urd::flat_vector<urd::index<Axiom<LiftedTag>>>>(),
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
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = DomainView;

        auto cls = nb::class_<V>(m, "Domain")  //
                       .def("get_index", &V::index)
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
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    ygg::bind_insert<Domain>(repository);
}

}  // namespace tyr::formalism::planning
