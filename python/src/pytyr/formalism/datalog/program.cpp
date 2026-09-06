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

namespace
{
template<TaskKind T>
void bind_program_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using Tag = Program<T>;
    ygg::bind_index<ygg::Index<Tag>>(m, (name + "Index").c_str());

    {
        using V = ygg::Data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str());
        cls.def(nb::init<const PredicateViewList<StaticTag>&,
                         const PredicateViewList<FluentTag>&,
                         const FunctionViewList<StaticTag>&,
                         const FunctionViewList<FluentTag>&,
                         const ObjectViewList&,
                         const AtomViewList<GroundTag, StaticTag>&,
                         const AtomViewList<GroundTag, FluentTag>&,
                         const FunctionTermValueViewList<GroundTag, StaticTag>&,
                         const FunctionTermValueViewList<GroundTag, FluentTag>&,
                         const std::optional<ConjunctiveConditionView<GroundTag>>&,
                         const std::optional<MetricView>&,
                         const RuleViewList<T, PredicateTag>&,
                         const RuleViewList<T, FunctionTag>&>(),
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
                nb::arg(std::same_as<T, GroundTag> ? "ground_rules" : "rules"),
                nb::arg(std::same_as<T, GroundTag> ? "ground_function_rules" : "function_rules"));
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = ProgramView<T>;
        auto cls = nb::class_<V>(m, name.c_str());
        cls.def("get_index", &V::get_index);
        cls.def("get_static_predicates", &V::template get_predicates<StaticTag>);
        cls.def("get_fluent_predicates", &V::template get_predicates<FluentTag>);
        cls.def("get_static_functions", &V::template get_functions<StaticTag>);
        cls.def("get_fluent_functions", &V::template get_functions<FluentTag>);
        cls.def("get_objects", &V::get_objects);
        cls.def("get_static_atoms", &V::template get_atoms<StaticTag>);
        cls.def("get_fluent_atoms", &V::template get_atoms<FluentTag>);
        cls.def("get_static_fterm_values", &V::template get_fterm_values<StaticTag>);
        cls.def("get_fluent_fterm_values", &V::template get_fterm_values<FluentTag>);
        cls.def("get_goal", &V::get_goal, nb::keep_alive<0, 1>());
        cls.def("get_metric", &V::get_metric);
        cls.def("get_rules", &V::template get_rules<PredicateTag>);
        cls.def("get_function_rules", &V::template get_rules<FunctionTag>);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Tag>, "data"_a, nb::keep_alive<0, 1>());
}
}  // namespace

void bind_program(nb::module_& m, RepositoryBinding& repository)
{
    bind_program_kind<LiftedTag>(m, repository, "Program");
    bind_program_kind<GroundTag>(m, repository, "GroundProgram");
}

}  // namespace tyr::formalism::datalog
