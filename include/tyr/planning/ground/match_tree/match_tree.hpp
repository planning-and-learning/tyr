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

#ifndef TYR_PLANNING_GROUND_MATCH_TREE_MATCH_TREE_HPP_
#define TYR_PLANNING_GROUND_MATCH_TREE_MATCH_TREE_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/nodes/node_data.hpp"

#include <optional>
#include <vector>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning::match_tree
{

template<typename Tag>
class MatchTree
{
private:
    ygg::IndexList<Tag> m_elements;

    RepositoryPtr<Tag> m_context;

    std::optional<ygg::View<ygg::Data<Node<Tag>>, Repository<Tag>>> m_root;

    std::vector<ygg::View<ygg::Data<Node<Tag>>, Repository<Tag>>> m_evaluate_stack;  ///< temporary during evaluation.

public:
    MatchTree(ygg::IndexList<Tag> elements, const ::tyr::formalism::planning::Repository& context);
    ~MatchTree();

    static MatchTreePtr<Tag> create(ygg::IndexList<Tag> elements, const ::tyr::formalism::planning::Repository& context);

    // Uncopieable and unmoveable to prohibit invalidating spans on m_elements.
    MatchTree(const MatchTree& other) = delete;
    MatchTree& operator=(const MatchTree& other) = delete;
    MatchTree(MatchTree&& other) = delete;
    MatchTree& operator=(MatchTree&& other) = delete;

    void generate(const StateContext<GroundTag>& state,
                  std::vector<ygg::View<ygg::Index<Tag>, ::tyr::formalism::planning::Repository>>& out_applicable_elements);
};

}

#endif
