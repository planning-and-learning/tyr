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

#include <tyr/formalism/datalog/ground_rule_data.hpp>
#include <tyr/formalism/datalog/ground_rule_index.hpp>
#include <tyr/formalism/datalog/ground_rule_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<RelationKind R>
void bind_ground_rule_data(nb::module_& m, const char* name)
{
    using Tag = GroundRule<R>;

    using V = ygg::Data<Tag>;
    auto cls = nb::class_<V>(m, name).def(nb::init<RuleBindingView<R>,
                                                   GroundConjunctiveConditionView,
                                                   typename V::template HeadView<Repository>,
                                                   const GroundNumericEffectOperatorViewList<FluentTag>&>(),
                                          "binding"_a,
                                          "body"_a,
                                          "head"_a,
                                          "metric_effects"_a = GroundNumericEffectOperatorViewList<FluentTag> {});
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<RelationKind R>
void bind_ground_rule_view(nb::module_& m, const char* name)
{
    using V = GroundRuleView<R>;
    auto cls = nb::class_<V>(m, name)
                   .def("get_index", &V::get_index)
                   .def("get_rule", &V::get_rule, nb::keep_alive<0, 1>())
                   .def("get_row", &V::get_row)
                   .def("get_objects", &V::get_objects)
                   .def("get_body", &V::get_body, nb::keep_alive<0, 1>())
                   .def("get_head", &V::get_head, nb::keep_alive<0, 1>())
                   .def("get_metric_effects", &V::get_metric_effects);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_ground_rule(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<GroundRule<PredicateTag>>>(m, "GroundRuleIndex");
    ygg::bind_index<ygg::Index<GroundRule<FunctionTag>>>(m, "GroundFunctionRuleIndex");

    bind_ground_rule_data<PredicateTag>(m, "GroundRuleData");
    bind_ground_rule_data<FunctionTag>(m, "GroundFunctionRuleData");

    bind_ground_rule_view<PredicateTag>(m, "GroundRule");
    bind_ground_rule_view<FunctionTag>(m, "GroundFunctionRule");

    repository.def("get_or_create", &get_or_create_data<GroundRule<PredicateTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<GroundRule<FunctionTag>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::datalog
