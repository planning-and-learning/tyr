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

#include <tyr/formalism/planning/numeric_effect_data.hpp>
#include <tyr/formalism/planning/numeric_effect_index.hpp>
#include <tyr/formalism/planning/numeric_effect_view.hpp>

namespace tyr::formalism::planning
{

namespace
{
template<NumericEffectOpKind Op, FactKind T>
void bind_numeric_effect_data(nb::module_& m, const char* name)
{
    using Tag = NumericEffect<Op, T>;

    using V = ygg::Data<Tag>;
    auto cls = nb::class_<V>(m, name).def(nb::init<FunctionTermView<T>, FunctionExpressionView>(), "fterm"_a, "fexpr"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<NumericEffectOpKind Op, FactKind T>
void bind_numeric_effect_view(nb::module_& m, const char* name)
{
    using V = NumericEffectView<Op, T>;
    auto cls = nb::class_<V>(m, name)
                   .def("get_index", &V::get_index)
                   .def("get_fterm", &V::get_fterm, nb::keep_alive<0, 1>())
                   .def("get_fexpr", &V::get_fexpr, nb::keep_alive<0, 1>());
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_numeric_effect(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<NumericEffect<Assign, FluentTag>>>(m, "FluentNumericEffectAssignIndex");
    ygg::bind_index<ygg::Index<NumericEffect<Increase, FluentTag>>>(m, "FluentNumericEffectIncreaseIndex");
    ygg::bind_index<ygg::Index<NumericEffect<Decrease, FluentTag>>>(m, "FluentNumericEffectDecreaseIndex");
    ygg::bind_index<ygg::Index<NumericEffect<ScaleUp, FluentTag>>>(m, "FluentNumericEffectScaleUpIndex");
    ygg::bind_index<ygg::Index<NumericEffect<ScaleDown, FluentTag>>>(m, "FluentNumericEffectScaleDownIndex");
    ygg::bind_index<ygg::Index<NumericEffect<Increase, AuxiliaryTag>>>(m, "AuxiliaryNumericEffectIncreaseIndex");

    bind_numeric_effect_data<Assign, FluentTag>(m, "FluentNumericEffectAssignData");
    bind_numeric_effect_data<Increase, FluentTag>(m, "FluentNumericEffectIncreaseData");
    bind_numeric_effect_data<Decrease, FluentTag>(m, "FluentNumericEffectDecreaseData");
    bind_numeric_effect_data<ScaleUp, FluentTag>(m, "FluentNumericEffectScaleUpData");
    bind_numeric_effect_data<ScaleDown, FluentTag>(m, "FluentNumericEffectScaleDownData");
    bind_numeric_effect_data<Increase, AuxiliaryTag>(m, "AuxiliaryNumericEffectIncreaseData");

    bind_numeric_effect_view<Assign, FluentTag>(m, "FluentNumericEffectAssign");
    bind_numeric_effect_view<Increase, FluentTag>(m, "FluentNumericEffectIncrease");
    bind_numeric_effect_view<Decrease, FluentTag>(m, "FluentNumericEffectDecrease");
    bind_numeric_effect_view<ScaleUp, FluentTag>(m, "FluentNumericEffectScaleUp");
    bind_numeric_effect_view<ScaleDown, FluentTag>(m, "FluentNumericEffectScaleDown");
    bind_numeric_effect_view<Increase, AuxiliaryTag>(m, "AuxiliaryNumericEffectIncrease");

    repository.def("get_or_create", &get_or_create_data<NumericEffect<Assign, FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<NumericEffect<Increase, FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<NumericEffect<Decrease, FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<NumericEffect<ScaleUp, FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<NumericEffect<ScaleDown, FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<NumericEffect<Increase, AuxiliaryTag>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::planning
