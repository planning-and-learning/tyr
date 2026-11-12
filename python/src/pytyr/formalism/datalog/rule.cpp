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
#include "pytyr/module.hpp"
#include "tyr/formalism/datalog/declarations.hpp"

#include <tyr/formalism/datalog/rule_data.hpp>
#include <tyr/formalism/datalog/rule_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<TaskKind T>
void bind_rule_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using namespace nb::literals;
    using Tag = Rule<T>;
    urd::python::bind_index<urd::index<Tag>>(m, name + "Index");

    {
        using V = urd::data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str());
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def(urd::python::init_from_views<const VariableViewList&, ConjunctiveConditionView<T>, const AtomViewList<T, FluentTag>&, ygg::float_t>(),
                    "variables"_a,
                    "body"_a,
                    "heads"_a,
                    "cost"_a = static_cast<ygg::float_t>(0));
            cls.def(nb::init<urd::flat_vector<urd::index<Variable>>,
                             urd::index<ConjunctiveCondition<LiftedTag>>,
                             urd::flat_vector<urd::index<Atom<LiftedTag, FluentTag>>>,
                             ygg::float_t>(),
                    "variables"_a,
                    "body"_a,
                    "heads"_a,
                    "cost"_a = static_cast<ygg::float_t>(0));
        }
        else
        {
            cls.def(urd::python::init_from_views<RuleRowView, ConjunctiveConditionView<T>, const AtomViewList<T, FluentTag>&, ygg::float_t>(),
                    "binding"_a,
                    "body"_a,
                    "heads"_a,
                    "cost"_a = static_cast<ygg::float_t>(0));
            cls.def(nb::init<ygg::RowIndex<Rule<LiftedTag>>,
                             urd::index<ConjunctiveCondition<GroundTag>>,
                             urd::flat_vector<urd::index<Atom<GroundTag, FluentTag>>>,
                             ygg::float_t>(),
                    "binding"_a,
                    "body"_a,
                    "heads"_a,
                    "cost"_a = static_cast<ygg::float_t>(0));
        }
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = RuleView<T>;
        auto cls = nb::class_<V>(m, name.c_str());
        cls.def("index", &V::index);
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
        cls.def("get_heads", &V::get_heads);
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
    bind_rule_kind<LiftedTag>(m, repository, "Rule");
    bind_rule_kind<GroundTag>(m, repository, "GroundRule");
}

}  // namespace tyr::formalism::datalog
