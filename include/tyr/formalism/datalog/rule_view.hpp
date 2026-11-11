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

#ifndef TYR_FORMALISM_DATALOG_RULE_VIEW_HPP_
#define TYR_FORMALISM_DATALOG_RULE_VIEW_HPP_

#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <ranges>
#include <urd/elements/flat.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/formalism/values.hpp>

namespace urd
{
template<::tyr::TaskKind T, ::tyr::formalism::RelationKind R, typename V>
struct view_mixin<::tyr::formalism::datalog::Rule<T, R>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto get_arity() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return get_body().get_arity(); }
    /// Its rows' column types: one object per variable (a key of row pools, as the repository declares its pool).
    auto get_column_types() const
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return std::views::iota(std::size_t { 0 }, std::size_t(get_arity()))
               | std::views::transform([](std::size_t) { return ygg::type_id<urd::index<::tyr::formalism::Object>>; });
    }
    auto get_variables() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::variables>(); }
    auto get_body() const noexcept { return self().template member<&urd::record_of<V>::body>(); }
    auto get_heads() const noexcept
        requires std::same_as<R, ::tyr::formalism::PredicateTag>
    { return self().template member<&urd::record_of<V>::heads>(); }
    auto get_head() const noexcept
        requires std::same_as<R, ::tyr::formalism::FunctionTag>
    { return self().template member<&urd::record_of<V>::head>(); }
    auto get_cost() const noexcept { return self().data().cost; }

    auto get_rule() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return get_row().get_relation(); }
    auto get_row() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::row>(); }
    /// Its objects: its row's columns read as objects (checked against its pool's types), resolved through the row's
    /// context.
    auto get_objects() const
        requires std::same_as<T, ::tyr::GroundTag>
    {
        const auto row = get_row();
        return urd::resolve(row.template get<urd::index<::tyr::formalism::Object>>(), row.context());
    }
    auto get_key() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return get_row().get_key(); }
};
}  // namespace urd

namespace tyr::formalism::datalog
{

/// Calls f for each head of a predicate rule, or for the single head of a function rule.
template<TaskKind T, RelationKind R, typename C, typename F>
void for_each_head(urd::index_view<Rule<T, R>, C> rule, F f)
{
    if constexpr (std::same_as<R, PredicateTag>)
        for (const auto head : rule.get_heads())
            f(head);
    else
        f(rule.get_head());
}

}

#endif
