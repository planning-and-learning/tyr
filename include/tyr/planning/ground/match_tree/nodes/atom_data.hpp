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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_NODES_ATOM_DATA_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_NODES_ATOM_DATA_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"

#include <optional>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include <urd/elements/flat.hpp>

namespace urd
{
template<typename Tag>
struct data<::tyr::planning::match_tree::AtomSelectorNode<Tag>>
{
    urd::index<::tyr::planning::match_tree::AtomSelectorNode<Tag>> index;
    urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> atom;
    urd::flat_optional<::tyr::planning::match_tree::Node<Tag>> true_child;
    urd::flat_optional<::tyr::planning::match_tree::Node<Tag>> false_child;
    urd::flat_optional<::tyr::planning::match_tree::Node<Tag>> dontcare_child;

    data() = default;
    data(urd::index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> atom_,
         urd::flat_optional<::tyr::planning::match_tree::Node<Tag>> true_child_,
         urd::flat_optional<::tyr::planning::match_tree::Node<Tag>> false_child_,
         urd::flat_optional<::tyr::planning::match_tree::Node<Tag>> dontcare_child_) :
        index(),
        atom(atom_),
        true_child(std::move(true_child_)),
        false_child(std::move(false_child_)),
        dontcare_child(std::move(dontcare_child_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;

    static constexpr auto cista_members() noexcept
    { return urd::Members<&data::index, &data::atom, &data::true_child, &data::false_child, &data::dontcare_child> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::atom, &data::true_child, &data::false_child, &data::dontcare_child> {}; }
};
}

#endif
