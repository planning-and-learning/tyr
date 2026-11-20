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

#include "tyr/formalism/formatter.hpp"

#include "tyr/formalism/enums.hpp"
#include "tyr/formalism/parameter_index.hpp"

#include <fmt/format.h>

fmt::format_context::iterator fmt::formatter<tyr::formalism::ParameterIndex>::format(const tyr::formalism::ParameterIndex& value,
                                                                                     fmt::format_context& context) const
{ return fmt::format_to(context.out(), "?x{}", value.value); }

fmt::format_context::iterator fmt::formatter<tyr::formalism::BooleanOperatorKind>::format(tyr::formalism::BooleanOperatorKind value,
                                                                                          fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", tyr::formalism::to_string(value)); }

fmt::format_context::iterator fmt::formatter<tyr::formalism::ArithmeticOperatorKind>::format(tyr::formalism::ArithmeticOperatorKind value,
                                                                                             fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", tyr::formalism::to_string(value)); }

fmt::format_context::iterator fmt::formatter<tyr::formalism::NumericEffectOperatorKind>::format(tyr::formalism::NumericEffectOperatorKind value,
                                                                                                fmt::format_context& context) const
{ return fmt::format_to(context.out(), "{}", tyr::formalism::to_string(value)); }
