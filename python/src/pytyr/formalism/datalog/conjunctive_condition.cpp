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
void bind_conjunctive_condition_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using Tag = ConjunctiveCondition<T>;
    ygg::bind_index<ygg::Index<Tag>>(m, (name + "Index").c_str());

    {
        using V = ygg::Data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str());
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def(nb::init<const VariableViewList&,
                             const LiteralViewList<T, StaticTag>&,
                             const LiteralViewList<T, FluentTag>&,
                             const LiftedBooleanOperatorViewList&>(),
                    "variables"_a,
                    "static_literals"_a,
                    "fluent_literals"_a,
                    "numeric_constraints"_a);
        }
        else
        {
            cls.def(nb::init<const LiteralViewList<T, StaticTag>&, const LiteralViewList<T, FluentTag>&, const GroundBooleanOperatorViewList&>(),
                    "static_literals"_a,
                    "fluent_literals"_a,
                    "numeric_constraints"_a);
        }
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = ConjunctiveConditionView<T>;
        auto cls = nb::class_<V>(m, name.c_str());
        cls.def("get_index", &V::get_index);
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def("get_variables", &V::get_variables);
            cls.def("get_arity", &V::get_arity);
        }
        cls.def("get_static_literals", &V::template get_literals<StaticTag>);
        cls.def("get_fluent_literals", &V::template get_literals<FluentTag>);
        cls.def("get_numeric_constraints", &V::get_numeric_constraints);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Tag>, "data"_a, nb::keep_alive<0, 1>());
}
}  // namespace

void bind_conjunctive_condition(nb::module_& m, RepositoryBinding& repository)
{
    bind_conjunctive_condition_kind<LiftedTag>(m, repository, "ConjunctiveCondition");
    bind_conjunctive_condition_kind<GroundTag>(m, repository, "GroundConjunctiveCondition");
}

}  // namespace tyr::formalism::datalog
