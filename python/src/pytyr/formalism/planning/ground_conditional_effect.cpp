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

void bind_ground_conditional_effect(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<GroundConditionalEffect>>(m, "GroundConditionalEffectIndex");

    {
        using V = ygg::Data<GroundConditionalEffect>;

        auto cls = nb::class_<V>(m, "GroundConditionalEffectData")  //
                       .def(nb::init<GroundConjunctiveConditionView, GroundConjunctiveEffectView>(), "condition"_a, "effect"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = GroundConditionalEffectView;

        auto cls = nb::class_<V>(m, "GroundConditionalEffect")  //
                       .def("get_index", &V::get_index)
                       .def("get_condition", &V::get_condition, nb::keep_alive<0, 1>())
                       .def("get_effect", &V::get_effect, nb::keep_alive<0, 1>());
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<GroundConditionalEffect>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::planning
