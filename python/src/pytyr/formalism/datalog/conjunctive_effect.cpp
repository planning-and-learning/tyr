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
void bind_conjunctive_effect_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using Tag = ConjunctiveEffect<T>;
    ygg::bind_index<ygg::Index<Tag>>(m, (name + "Index").c_str());

    {
        using V = ygg::Data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str());
        cls.def(nb::init<const NumericEffectOperatorViewList<T, FluentTag>&>(), "numeric_effects"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = ConjunctiveEffectView<T>;
        auto cls = nb::class_<V>(m, name.c_str());
        cls.def("get_index", &V::get_index);
        cls.def("get_numeric_effects", &V::get_numeric_effects);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Tag>, "data"_a, nb::keep_alive<0, 1>());
}
}  // namespace

void bind_conjunctive_effect(nb::module_& m, RepositoryBinding& repository)
{
    bind_conjunctive_effect_kind<LiftedTag>(m, repository, "ConjunctiveEffect");
    bind_conjunctive_effect_kind<GroundTag>(m, repository, "GroundConjunctiveEffect");
}

}  // namespace tyr::formalism::datalog
