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

#include <tyr/formalism/datalog/ground_numeric_effect_data.hpp>
#include <tyr/formalism/datalog/ground_numeric_effect_index.hpp>
#include <tyr/formalism/datalog/ground_numeric_effect_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<NumericEffectOpKind Op, FactKind T>
void bind_ground_numeric_effect_data(nb::module_& m, const char* name)
{
    using Tag = GroundNumericEffect<Op, T>;

    using V = ygg::Data<Tag>;
    auto cls = nb::class_<V>(m, name).def(nb::init<GroundFunctionTermView<T>, GroundFunctionExpressionView>(), "fterm"_a, "fexpr"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<NumericEffectOpKind Op, FactKind T>
void bind_ground_numeric_effect_view(nb::module_& m, const char* name)
{
    using V = GroundNumericEffectView<Op, T>;
    auto cls = nb::class_<V>(m, name)
                   .def("get_index", &V::get_index)
                   .def("get_fterm", &V::get_fterm, nb::keep_alive<0, 1>())
                   .def("get_fexpr", &V::get_fexpr, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_ground_numeric_effect(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<GroundNumericEffect<Assign, FluentTag>>>(m, "FluentGroundNumericEffectAssignIndex");
    ygg::bind_index<ygg::Index<GroundNumericEffect<Increase, FluentTag>>>(m, "FluentGroundNumericEffectIncreaseIndex");
    ygg::bind_index<ygg::Index<GroundNumericEffect<Decrease, FluentTag>>>(m, "FluentGroundNumericEffectDecreaseIndex");
    ygg::bind_index<ygg::Index<GroundNumericEffect<ScaleUp, FluentTag>>>(m, "FluentGroundNumericEffectScaleUpIndex");
    ygg::bind_index<ygg::Index<GroundNumericEffect<ScaleDown, FluentTag>>>(m, "FluentGroundNumericEffectScaleDownIndex");

    bind_ground_numeric_effect_data<Assign, FluentTag>(m, "FluentGroundNumericEffectAssignData");
    bind_ground_numeric_effect_data<Increase, FluentTag>(m, "FluentGroundNumericEffectIncreaseData");
    bind_ground_numeric_effect_data<Decrease, FluentTag>(m, "FluentGroundNumericEffectDecreaseData");
    bind_ground_numeric_effect_data<ScaleUp, FluentTag>(m, "FluentGroundNumericEffectScaleUpData");
    bind_ground_numeric_effect_data<ScaleDown, FluentTag>(m, "FluentGroundNumericEffectScaleDownData");

    bind_ground_numeric_effect_view<Assign, FluentTag>(m, "FluentGroundNumericEffectAssign");
    bind_ground_numeric_effect_view<Increase, FluentTag>(m, "FluentGroundNumericEffectIncrease");
    bind_ground_numeric_effect_view<Decrease, FluentTag>(m, "FluentGroundNumericEffectDecrease");
    bind_ground_numeric_effect_view<ScaleUp, FluentTag>(m, "FluentGroundNumericEffectScaleUp");
    bind_ground_numeric_effect_view<ScaleDown, FluentTag>(m, "FluentGroundNumericEffectScaleDown");

    repository.def("get_or_create", &get_or_create_data<GroundNumericEffect<Assign, FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<GroundNumericEffect<Increase, FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<GroundNumericEffect<Decrease, FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<GroundNumericEffect<ScaleUp, FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<GroundNumericEffect<ScaleDown, FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::datalog
