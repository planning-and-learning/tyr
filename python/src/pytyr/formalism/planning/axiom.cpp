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
void bind_axiom_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using Tag = Axiom<T>;
    ygg::bind_index<ygg::Index<Tag>>(m, (name + "Index").c_str());

    {
        using V = ygg::Data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str());
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def(nb::init<const VariableViewList&, ConjunctiveConditionView<T>, AtomView<T, DerivedTag>>(), "variables"_a, "body"_a, "head"_a);
        }
        else
        {
            cls.def(nb::init<AxiomBindingView, ConjunctiveConditionView<T>, AtomView<T, DerivedTag>>(), "binding"_a, "body"_a, "head"_a);
        }
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = AxiomView<T>;
        auto cls = nb::class_<V>(m, name.c_str());
        cls.def("get_index", &V::get_index);
        if constexpr (std::same_as<T, LiftedTag>)
        {
            cls.def("get_arity", &V::get_arity);
            cls.def("get_variables", &V::get_variables);
        }
        else
        {
            cls.def("get_axiom", &V::get_axiom, nb::keep_alive<0, 1>());
            cls.def("get_row", &V::get_row, nb::keep_alive<0, 1>());
            cls.def("get_objects", &V::get_objects);
        }
        cls.def("get_body", &V::get_body, nb::keep_alive<0, 1>());
        cls.def("get_head", &V::get_head, nb::keep_alive<0, 1>());
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Tag>, "data"_a, nb::keep_alive<0, 1>());
}
}  // namespace

void bind_axiom(nb::module_& m, RepositoryBinding& repository)
{
    bind_axiom_kind<LiftedTag>(m, repository, "Axiom");
    bind_axiom_kind<GroundTag>(m, repository, "GroundAxiom");
}

}  // namespace tyr::formalism::planning
