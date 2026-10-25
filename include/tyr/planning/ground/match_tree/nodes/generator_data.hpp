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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_GENERATOR_DATA_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_GENERATOR_DATA_HPP_

#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_data.hpp"

#include <optional>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
template<typename Tag>
struct Data<::tyr::planning::match_tree::ElementGeneratorNode<Tag>>
{
    ygg::Index<::tyr::planning::match_tree::ElementGeneratorNode<Tag>> index;
    ygg::IndexList<Tag> elements;

    Data() = default;
    Data(ygg::IndexList<Tag> elements_) : index(), elements(std::move(elements_)) {}
    template<typename C>
    Data(const std::vector<::ygg::View<ygg::Index<Tag>, C>>& elements_) : index(), elements()
    { set(elements_, elements); }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;
    ~Data() = default;

    static constexpr auto cista_members() noexcept { return ygg::Members<&Data::index, &Data::elements> {}; }
    static constexpr auto identifying_members() noexcept { return ygg::Members<&Data::elements> {}; }
    void clear() noexcept { ygg::clear_members(*this); }
};
}

#endif
