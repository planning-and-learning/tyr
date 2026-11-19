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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_GENERATOR_BUILDER_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_GENERATOR_BUILDER_HPP_

#include "tyr/planning/ground/match_tree/nodes/generator_data.hpp"

#include <optional>
#include <urd/elements/builder.hpp>
#include <urd/elements/children.hpp>
#include <variant>
#include <vector>

namespace urd
{

/// The builder of ElementGeneratorNode: its record's members, each in its built form (urd::built_t).
template<typename Tag, typename C>
struct builder<::tyr::planning::match_tree::ElementGeneratorNode<Tag>, C>
{
    urd::index<::tyr::planning::match_tree::ElementGeneratorNode<Tag>> index;
    urd::built_t<urd::flat_vector<urd::index<Tag>>, ::tyr::formalism::planning::Repository> elements;

    static constexpr auto fields() noexcept { return urd::Fields<urd::field<"elements", &builder::elements>> {}; }

    static constexpr auto identifying_members() noexcept { return urd::Members<&builder::elements> {}; }
};

}  // namespace urd

#endif
