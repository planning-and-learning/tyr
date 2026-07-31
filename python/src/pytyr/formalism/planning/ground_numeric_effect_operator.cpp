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

#include <tyr/formalism/planning/ground_numeric_effect_operator_data.hpp>
#include <tyr/formalism/planning/ground_numeric_effect_operator_view.hpp>

namespace tyr::formalism::planning
{

void bind_ground_numeric_effect_operator(nb::module_& m, RepositoryBinding& repository)
{
    {
        using V = ygg::Data<GroundNumericEffectOperator<FluentTag>>;
        auto cls = nb::class_<V>(m, "FluentGroundNumericEffectOperatorData").def(nb::init<V::ViewVariant<Repository>>(), "value"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using V = ygg::Data<GroundNumericEffectOperator<AuxiliaryTag>>;
        auto cls = nb::class_<V>(m, "AuxiliaryGroundNumericEffectOperatorData").def(nb::init<V::ViewVariant<Repository>>(), "value"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using V = GroundNumericEffectOperatorView<FluentTag>;
        auto cls = nb::class_<V>(m, "FluentGroundNumericEffectOperator").def("get_variant", &V::get_variant);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }
    {
        using V = GroundNumericEffectOperatorView<AuxiliaryTag>;
        auto cls = nb::class_<V>(m, "AuxiliaryGroundNumericEffectOperator").def("get_variant", &V::get_variant);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("create", &create_data<GroundNumericEffectOperator<FluentTag>>, "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>());
    repository.def("create", &create_data<GroundNumericEffectOperator<AuxiliaryTag>>, "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>());
}

}  // namespace tyr::formalism::planning
