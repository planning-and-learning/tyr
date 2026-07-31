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

#include <tyr/formalism/datalog/numeric_effect_operator_data.hpp>
#include <tyr/formalism/datalog/numeric_effect_operator_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<FactKind T>
void bind_numeric_effect_operator_data(nb::module_& m, const char* name)
{
    using Tag = NumericEffectOperator<T>;

    using V = ygg::Data<Tag>;
    auto cls = nb::class_<V>(m, name).def(nb::init<typename V::template ViewVariant<Repository>>(), "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_numeric_effect_operator_view(nb::module_& m, const char* name)
{
    using V = NumericEffectOperatorView<T>;
    auto cls = nb::class_<V>(m, name).def("get_variant", &V::get_variant);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_numeric_effect_operator(nb::module_& m, RepositoryBinding& repository)
{
    bind_numeric_effect_operator_data<FluentTag>(m, "FluentNumericEffectOperatorData");

    bind_numeric_effect_operator_view<FluentTag>(m, "FluentNumericEffectOperator");

    repository.def("create", &create_data<NumericEffectOperator<FluentTag>>, "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>());
}

}  // namespace tyr::formalism::datalog
