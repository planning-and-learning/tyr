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

#include "tyr/formalism/planning/atom_index.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/atom_index.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_data.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{
using namespace ::tyr;

template<typename Tag>
struct Data<planning::match_tree::AtomSelectorNode<Tag>>
{
    ygg::Index<planning::match_tree::AtomSelectorNode<Tag>> index;
    ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> atom;
    ::cista::optional<ygg::Data<planning::match_tree::Node<Tag>>> true_child;
    ::cista::optional<ygg::Data<planning::match_tree::Node<Tag>>> false_child;
    ::cista::optional<ygg::Data<planning::match_tree::Node<Tag>>> dontcare_child;

    Data() = default;
    Data(ygg::Index<planning::match_tree::AtomSelectorNode<Tag>> index,
         ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::DerivedTag>> atom,
         ::cista::optional<ygg::Data<planning::match_tree::Node<Tag>>> true_child,
         ::cista::optional<ygg::Data<planning::match_tree::Node<Tag>>> false_child,
         ::cista::optional<ygg::Data<planning::match_tree::Node<Tag>>> dontcare_child) :
        index(index),
        atom(atom),
        true_child(true_child),
        false_child(false_child),
        dontcare_child(dontcare_child)
    {
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(atom);
        ygg::clear(true_child);
        ygg::clear(false_child);
        ygg::clear(dontcare_child);
    }

    auto cista_members() const noexcept { return std::tie(index, atom, true_child, false_child, dontcare_child); }
    auto identifying_members() const noexcept { return std::tie(atom, true_child, false_child, dontcare_child); }
};
}

#endif
