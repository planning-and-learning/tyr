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
#include "tyr/formalism/datalog/declarations.hpp"

#include <tyr/formalism/datalog/rule_data.hpp>
#include <tyr/formalism/datalog/rule_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<TaskKind T, RelationKind R>
void bind_rule_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using namespace nb::literals;
    using Tag = Rule<T, R>;
    urd::python::bind_index<urd::index<Tag>>(m, name + "Index");

    {
        using V = urd::data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str());
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def(urd::python::init_from_views<const VariableViewList&, ConjunctiveConditionView<T>, typename V::template HeadView<Repository>, ygg::float_t>(),
                    "variables"_a,
                    "body"_a,
                    std::same_as<R, PredicateTag> ? "heads"_a : "head"_a,
                    "cost"_a = static_cast<ygg::float_t>(0));
            cls.def(nb::init<urd::flat_vector<urd::index<Variable>>, urd::index<ConjunctiveCondition<LiftedTag>>, RuleHeadT<LiftedTag, R>, ygg::float_t>(),
                    "variables"_a,
                    "body"_a,
                    std::same_as<R, PredicateTag> ? "heads"_a : "head"_a,
                    "cost"_a = static_cast<ygg::float_t>(0));
        }
        else
        {
            cls.def(urd::python::init_from_views<RuleBindingView<R>, ConjunctiveConditionView<T>, typename V::template HeadView<Repository>, ygg::float_t>(),
                    "binding"_a,
                    "body"_a,
                    std::same_as<R, PredicateTag> ? "heads"_a : "head"_a,
                    "cost"_a = static_cast<ygg::float_t>(0));
            cls.def(
                nb::init<ygg::formalism::RelationBindingIndex<Rule<LiftedTag, R>, ::tyr::formalism::ObjectTag>, urd::index<ConjunctiveCondition<GroundTag>>, RuleHeadT<GroundTag, R>, ygg::float_t>(),
                "binding"_a,
                "body"_a,
                std::same_as<R, PredicateTag> ? "heads"_a : "head"_a,
                "cost"_a = static_cast<ygg::float_t>(0));
        }
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = RuleView<T, R>;
        auto cls = nb::class_<V>(m, name.c_str());
        cls.def("get_index", &V::index);
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def("get_arity", &V::get_arity);
            cls.def("get_variables", &V::get_variables);
        }
        else
        {
            cls.def("get_rule", &V::get_rule);
            cls.def("get_row", &V::get_row);
            cls.def("get_objects", &V::get_objects);
        }
        cls.def("get_body", &V::get_body);
        if constexpr (std::same_as<R, PredicateTag>)
            cls.def("get_heads", &V::get_heads);
        else
            cls.def("get_head", &V::get_head);
        cls.def("get_cost", &V::get_cost);
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    ygg::bind_insert<Tag>(repository);
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
