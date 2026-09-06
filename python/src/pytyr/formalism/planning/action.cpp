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

namespace
{
template<TaskKind T>
void bind_action_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using Tag = Action<T>;
    ygg::bind_index<ygg::Index<Tag>>(m, (name + "Index").c_str());

    {
        using V = ygg::Data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str());
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def(nb::init<const std::string&, ygg::uint_t, const VariableViewList&, ConjunctiveConditionView<T>, const ConditionalEffectViewList<T>&>(),
                    "name"_a,
                    "original_arity"_a,
                    "variables"_a,
                    "condition"_a,
                    "effects"_a);
            cls.def(nb::init<const std::string&,
                             const std::string&,
                             ygg::uint_t,
                             const VariableViewList&,
                             ConjunctiveConditionView<T>,
                             const ConditionalEffectViewList<T>&>(),
                    "name"_a,
                    "original_name"_a,
                    "original_arity"_a,
                    "variables"_a,
                    "condition"_a,
                    "effects"_a);
        }
        else
        {
            cls.def(nb::init<ActionBindingView, ConjunctiveConditionView<T>, const ConditionalEffectViewList<T>&>(), "binding"_a, "condition"_a, "effects"_a);
        }
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = ActionView<T>;
        auto cls = nb::class_<V>(m, name.c_str());
        cls.def("get_index", &V::get_index);
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def("get_name", &V::get_name);
            cls.def("get_original_name", &V::get_original_name);
            cls.def("get_original_arity", &V::get_original_arity);
            cls.def("get_arity", &V::get_arity);
            cls.def("get_variables", &V::get_variables);
        }
        else
        {
            cls.def("get_action", &V::get_action, nb::keep_alive<0, 1>());
            cls.def("get_row", &V::get_row, nb::keep_alive<0, 1>());
            cls.def("get_objects", &V::get_objects);
        }
        cls.def("get_condition", &V::get_condition, nb::keep_alive<0, 1>());
        cls.def("get_effects", &V::get_effects);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Tag>, "data"_a, nb::keep_alive<0, 1>());
}
}  // namespace

void bind_action(nb::module_& m, RepositoryBinding& repository)
{
    bind_action_kind<LiftedTag>(m, repository, "Action");
    bind_action_kind<GroundTag>(m, repository, "GroundAction");
}

}  // namespace tyr::formalism::planning
