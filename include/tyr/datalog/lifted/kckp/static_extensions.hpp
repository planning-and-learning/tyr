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
#include "tyr/datalog/lifted/kckp/workspaces/rule.hpp"

#include <span>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::datalog
{

/// Scratch of for_each_static_extension.
struct StaticExtensionWorkspace
{
    urd::Store<urd::sequence<kckp::Vertex>> prefix;
    kckp::Workspace kckp;
    urd::flat_vector<urd::index<formalism::Object>> extension;
};

/// Calls `callback` with the objects of the rule's remaining variables for each binding that extends the objects of its first
/// variables and is consistent with the rule's static consistency graph: the pairwise static constraints of its body.
template<typename Callback>
void for_each_static_extension(const ConstRuleWorkspace<LiftedTag>& rule,
                               std::span<const urd::index<formalism::Object>> prefix,
                               StaticExtensionWorkspace& workspace,
                               Callback callback)
{
    const auto& graph = rule.get_static_consistency_graph().get_graph();
    assert(prefix.size() <= graph.num_parts());

    workspace.prefix.clear();
    for (ygg::uint_t partition = 0; partition < prefix.size(); ++partition)
    {
        const auto vertex = graph.find_vertex(partition, static_cast<ygg::uint_t>(prefix[partition].value));
        if (!vertex)
            return;
        workspace.prefix.push_back(*vertex);
    }

    kckp::KCKP(graph).for_each_compatible_extension(
        workspace.prefix,
        workspace.kckp,
        [&](std::span<const kckp::Vertex> extension)
        {
            workspace.extension.clear();
            for (const auto vertex : extension)
                workspace.extension.push_back(urd::index<formalism::Object>(graph.label(vertex)));
            callback(std::span<const urd::index<formalism::Object>>(workspace.extension.data(), workspace.extension.size()));
        });
}

}

#endif
