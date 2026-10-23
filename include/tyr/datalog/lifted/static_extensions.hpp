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

#ifndef TYR_DATALOG_LIFTED_STATIC_EXTENSIONS_HPP_
#define TYR_DATALOG_LIFTED_STATIC_EXTENSIONS_HPP_

#include "tyr/algorithms/kckp/kckp.hpp"
#include "tyr/datalog/lifted/workspaces/rule.hpp"

#include <span>
#include <vector>

namespace tyr::datalog
{

/// Scratch of for_each_static_extension.
struct StaticExtensionWorkspace
{
    std::vector<kckp::Vertex> prefix;
    kckp::Workspace kckp;
    ygg::IndexList<formalism::Object> extension;
};

/// Calls `callback` with the objects of the rule's remaining variables for each binding that extends the objects of its first
/// variables and is consistent with the rule's static consistency graph: the pairwise static constraints of its body.
template<formalism::RelationKind R, typename Callback>
void for_each_static_extension(const ConstRuleWorkspace<LiftedTag, R>& rule,
                               std::span<const ygg::Index<formalism::Object>> prefix,
                               StaticExtensionWorkspace& workspace,
                               Callback&& callback)
{
    const auto& graph = rule.get_static_consistency_graph().get_graph();
    const auto& layout = graph.get_layout();
    assert(prefix.size() <= layout.num_partitions);

    workspace.prefix.clear();
    for (ygg::uint_t partition = 0; partition < prefix.size(); ++partition)
    {
        const auto vertex = layout.find_vertex(partition, ygg::uint_t(prefix[partition]));
        if (!vertex)
            return;
        workspace.prefix.push_back(*vertex);
    }

    kckp::KCKP(graph).for_each_compatible_extension(workspace.prefix,
                                                    workspace.kckp,
                                                    [&](std::span<const kckp::Vertex> extension)
                                                    {
                                                        workspace.extension.clear();
                                                        for (const auto vertex : extension)
                                                            workspace.extension.push_back(ygg::Index<formalism::Object>(layout.vertex_labels[vertex.index]));
                                                        callback(std::span<const ygg::Index<formalism::Object>>(workspace.extension.data(),
                                                                                                                 workspace.extension.size()));
                                                    });
}

}

#endif
