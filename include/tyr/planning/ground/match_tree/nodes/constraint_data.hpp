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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_CONSTRAINT_DATA_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_CONSTRAINT_DATA_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/enums.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"

#include <urd/elements/concepts.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <utility>

namespace urd
{
template<typename Tag>
struct data<::tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>
{
    urd::key<::tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>> index;
    urd::index<::tyr::formalism::BinaryOperator<::tyr::GroundTag, ::tyr::formalism::BooleanOperatorKind>> constraint;
    urd::flat_optional<::tyr::planning::match_tree::Node<Tag>> true_child;
    urd::flat_optional<::tyr::planning::match_tree::Node<Tag>> dontcare_child;

    data() = default;

    data(urd::index<::tyr::formalism::BinaryOperator<::tyr::GroundTag, ::tyr::formalism::BooleanOperatorKind>> constraint_,
         urd::flat_optional<::tyr::planning::match_tree::Node<Tag>> true_child_,
         urd::flat_optional<::tyr::planning::match_tree::Node<Tag>> dontcare_child_) :
        index(),
        constraint(constraint_),
        true_child(std::move(true_child_)),
        dontcare_child(std::move(dontcare_child_))
    {
    }

    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"constraint", &data::constraint>,
                           urd::field<"true_child", &data::true_child>,
                           urd::field<"dontcare_child", &data::dontcare_child>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&data::constraint, &data::true_child, &data::dontcare_child> {}; }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::constraint, &data::true_child, &data::dontcare_child> {}; }

    [[nodiscard]] urd::key<::tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>> key() const noexcept
    { return urd::key<::tyr::planning::match_tree::NumericConstraintSelectorNode<Tag>>(index); }
};
}

#endif
