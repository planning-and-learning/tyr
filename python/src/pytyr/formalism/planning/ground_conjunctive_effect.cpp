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

void bind_ground_conjunctive_effect(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<GroundConjunctiveEffect>>(m, "GroundConjunctiveEffectIndex");

    {
        using V = ygg::Data<GroundConjunctiveEffect>;

        auto cls = nb::class_<V>(m, "GroundConjunctiveEffectData")  //
                       .def(nb::init<const FDRFactViewList<FluentTag>&,
                                     const FDRFactViewList<FluentTag>&,
                                     const GroundNumericEffectOperatorViewList<FluentTag>&,
                                     const std::optional<GroundNumericEffectOperatorView<AuxiliaryTag>>&>(),
                            "add_facts"_a,
                            "del_facts"_a,
                            "fluent_numeric_effects"_a,
                            "auxiliary_numeric_effect"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = GroundConjunctiveEffectView;

        auto cls = nb::class_<V>(m, "GroundConjunctiveEffect")  //
                       .def("get_index", &V::get_index)
                       .def("get_add_facts", &V::get_facts<PositiveTag>)
                       .def("get_del_facts", &V::get_facts<NegativeTag>)
                       .def("get_numeric_effects", &V::get_numeric_effects)
                       .def("get_auxiliary_numeric_effect", &V::get_auxiliary_numeric_effect);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<GroundConjunctiveEffect>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::planning
