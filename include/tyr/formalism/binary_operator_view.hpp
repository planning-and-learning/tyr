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

#ifndef TYR_FORMALISM_BINARY_OPERATOR_VIEW_HPP_
#define TYR_FORMALISM_BINARY_OPERATOR_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <fmt/base.h>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace urd
{
template<::tyr::TaskKind T, ::tyr::formalism::OperatorKind O, typename V>
struct view_mixin<::tyr::formalism::BinaryOperator<T, O>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto operation() const noexcept { return self().template member<&urd::record_of<V>::operation>(); }

    auto left_operand() const noexcept { return self().template member<&urd::record_of<V>::left_operand>(); }

    auto right_operand() const noexcept { return self().template member<&urd::record_of<V>::right_operand>(); }
};
}  // namespace urd

/// A binary arithmetic expression (+ a b), or a numeric comparison (>= a b); != is (not (= a b)).
template<tyr::TaskKind T, tyr::formalism::OperatorKind O>
struct fmt::formatter<urd::index_view<tyr::formalism::BinaryOperator<T, O>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::BinaryOperator<T, O>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

#endif
