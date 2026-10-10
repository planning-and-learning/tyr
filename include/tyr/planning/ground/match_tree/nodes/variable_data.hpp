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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_VARIABLE_DATA_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_VARIABLE_DATA_HPP_

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
using namespace ::tyr;

template<typename Tag>
struct Data<planning::match_tree::VariableSelectorNode<Tag>>
{
    ygg::Index<planning::match_tree::VariableSelectorNode<Tag>> index;
    ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> variable;
    ::cista::offset::vector<::cista::optional<ygg::Data<planning::match_tree::Node<Tag>>>> domain_children;
    ::cista::optional<ygg::Data<planning::match_tree::Node<Tag>>> dontcare_child;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>> variable_,
         ::cista::offset::vector<::cista::optional<ygg::Data<planning::match_tree::Node<Tag>>>> domain_children_,
         ::cista::optional<ygg::Data<planning::match_tree::Node<Tag>>> dontcare_child_) :
        index(),
        variable(variable_),
        domain_children(std::move(domain_children_)),
        dontcare_child(std::move(dontcare_child_))
    {
    }
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::planning::FDRVariable<::tyr::formalism::FluentTag>>, C> variable_,
         const std::vector<std::optional<::ygg::View<ygg::Data<planning::match_tree::Node<Tag>>, C>>>& domain_children_,
         const std::optional<::ygg::View<ygg::Data<planning::match_tree::Node<Tag>>, C>>& dontcare_child_) :
        index(),
        variable(),
        domain_children(),
        dontcare_child()
    {
        set(variable_, variable);
        domain_children.reserve(domain_children_.size());
        for (const auto& child : domain_children_)
        {
            domain_children.emplace_back();
            set(child, domain_children.back());
        }
        set(dontcare_child_, dontcare_child);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    auto cista_members() noexcept { return std::tie(index, variable, domain_children, dontcare_child); }
    auto cista_members() const noexcept { return std::tie(index, variable, domain_children, dontcare_child); }
    auto identifying_members() const noexcept { return std::tie(variable, domain_children, dontcare_child); }
    void clear() noexcept
    {
        std::apply([](auto&... member) { (ygg::clear(member), ...); }, cista_members());
    }
};
}

#endif
