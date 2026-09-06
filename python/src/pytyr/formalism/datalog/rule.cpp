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

#include <tyr/formalism/datalog/rule_data.hpp>
#include <tyr/formalism/datalog/rule_index.hpp>
#include <tyr/formalism/datalog/rule_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<TaskKind T, RelationKind R>
void bind_rule_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using Tag = Rule<T, R>;
    ygg::bind_index<ygg::Index<Tag>>(m, (name + "Index").c_str());

    {
        using V = ygg::Data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str());
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def(nb::init<const VariableViewList&,
                             ConjunctiveConditionView<T>,
                             typename V::template HeadView<Repository>,
                             const NumericEffectOperatorViewList<T, FluentTag>&>(),
                    "variables"_a,
                    "body"_a,
                    "head"_a,
                    "metric_effects"_a = NumericEffectOperatorViewList<T, FluentTag> {});
        }
        else
        {
            cls.def(nb::init<RuleBindingView<R>,
                             ConjunctiveConditionView<T>,
                             typename V::template HeadView<Repository>,
                             const NumericEffectOperatorViewList<T, FluentTag>&>(),
                    "binding"_a,
                    "body"_a,
                    "head"_a,
                    "metric_effects"_a = NumericEffectOperatorViewList<T, FluentTag> {});
        }
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = RuleView<T, R>;
        auto cls = nb::class_<V>(m, name.c_str());
        cls.def("get_index", &V::get_index);
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def("get_arity", &V::get_arity);
            cls.def("get_variables", &V::get_variables);
        }
        else
        {
            cls.def("get_rule", &V::get_rule, nb::keep_alive<0, 1>());
            cls.def("get_row", &V::get_row);
            cls.def("get_objects", &V::get_objects);
        }
        cls.def("get_body", &V::get_body, nb::keep_alive<0, 1>());
        cls.def("get_head", &V::get_head, nb::keep_alive<0, 1>());
        cls.def("get_metric_effects", &V::get_metric_effects);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Tag>, "data"_a, nb::keep_alive<0, 1>());
}
}  // namespace

void bind_rule(nb::module_& m, RepositoryBinding& repository)
{
    bind_rule_kind<LiftedTag, PredicateTag>(m, repository, "Rule");
    bind_rule_kind<LiftedTag, FunctionTag>(m, repository, "FunctionRule");
    bind_rule_kind<GroundTag, PredicateTag>(m, repository, "GroundRule");
    bind_rule_kind<GroundTag, FunctionTag>(m, repository, "GroundFunctionRule");
}

}  // namespace tyr::formalism::datalog
