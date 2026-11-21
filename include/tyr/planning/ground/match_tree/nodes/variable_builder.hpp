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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_VARIABLE_BUILDER_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_VARIABLE_BUILDER_HPP_

#include "tyr/formalism/planning/fdr_variable_builder.hpp"
#include "tyr/planning/ground/match_tree/nodes/variable_data.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of VariableSelectorNode: its record's members, each in its built form (urd::built_t).
template<typename Tag, typename C>
struct builder<::tyr::planning::match_tree::VariableSelectorNode<Tag>, C>
{
    urd::index<::tyr::planning::match_tree::VariableSelectorNode<Tag>> index;
    urd::built_t<urd::index<::tyr::formalism::FDRVariable<::tyr::formalism::FluentTag>>, ::tyr::formalism::Repository> variable;
    urd::built_t<urd::flat_vector<urd::flat_optional<::tyr::planning::match_tree::Node<Tag>>>, C> domain_children;
    urd::built_t<urd::flat_optional<::tyr::planning::match_tree::Node<Tag>>, C> dontcare_child;

    static constexpr auto fields() noexcept
    {
        return urd::Fields<urd::field<"variable", &builder::variable>,
                           urd::field<"domain_children", &builder::domain_children>,
                           urd::field<"dontcare_child", &builder::dontcare_child>> {};
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::variable, &builder::domain_children, &builder::dontcare_child> {}; }
};

}  // namespace urd

#endif
