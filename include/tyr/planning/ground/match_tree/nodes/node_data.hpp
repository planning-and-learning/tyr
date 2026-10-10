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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_NODE_DATA_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_NODE_DATA_HPP_

#include "tyr/planning/ground/match_tree/declarations.hpp"

#include <yggdrasil/containers/variant.hpp>
#include <optional>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
using namespace ::tyr;

template<typename Tag>
struct Data<planning::match_tree::Node<Tag>>
{
    using Variant = ::ygg::IndexVariant<planning::match_tree::NodeTypes<Tag>>;

    Variant variant;

    template<typename C>
    using ViewVariant = ::ygg::ViewVariant<Variant, C>;

    Data() = default;
    Data(Variant variant_) : variant(variant_) {}
    template<typename C>
    Data(const ViewVariant<C>& variant_) : variant(std::visit([](const auto& view) -> Variant { return Variant(view.get_index()); }, variant_))
    {
    }

    auto cista_members() noexcept { return std::tie(variant); }
    auto cista_members() const noexcept { return std::tie(variant); }
    auto identifying_members() const noexcept { return std::tie(variant); }
    void clear() noexcept
    {
        std::apply([](auto&... member) { (ygg::clear(member), ...); }, cista_members());
    }
};

}

#endif
