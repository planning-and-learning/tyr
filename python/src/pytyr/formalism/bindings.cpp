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

#include "bindings.hpp"

#include <tyr/formalism/enums.hpp>

namespace tyr::formalism
{

void bind_formalism(nb::module_& m)
{
    nb::enum_<BooleanOperatorKind>(m, "BooleanOperatorKind")
        .value("Eq", BooleanOperatorKind::Eq)
        .value("Ne", BooleanOperatorKind::Ne)
        .value("Le", BooleanOperatorKind::Le)
        .value("Lt", BooleanOperatorKind::Lt)
        .value("Ge", BooleanOperatorKind::Ge)
        .value("Gt", BooleanOperatorKind::Gt);

    nb::enum_<ArithmeticOperatorKind>(m, "ArithmeticOperatorKind")
        .value("Add", ArithmeticOperatorKind::Add)
        .value("Sub", ArithmeticOperatorKind::Sub)
        .value("Mul", ArithmeticOperatorKind::Mul)
        .value("Div", ArithmeticOperatorKind::Div);

    nb::enum_<NumericEffectOperatorKind>(m, "NumericEffectOperatorKind")
        .value("Assign", NumericEffectOperatorKind::Assign)
        .value("Increase", NumericEffectOperatorKind::Increase)
        .value("Decrease", NumericEffectOperatorKind::Decrease)
        .value("ScaleUp", NumericEffectOperatorKind::ScaleUp)
        .value("ScaleDown", NumericEffectOperatorKind::ScaleDown);

    bind_parameter(m);
    bind_row(m);
    bind_object(m);
    bind_variable(m);
    bind_binding(m);
    bind_predicate(m);
    bind_function(m);
    bind_term(m);
}

}  // namespace tyr::formalism
