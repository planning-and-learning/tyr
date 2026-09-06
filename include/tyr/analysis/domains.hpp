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

#ifndef TYR_ANALYSIS_DOMAINS_HPP_
#define TYR_ANALYSIS_DOMAINS_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/analysis/program_analysis.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/object_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <cassert>
#include <ranges>
#include <span>
#include <utility>
#include <vector>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::analysis
{
ProgramVariableDomains compute_variable_domains(formalism::datalog::ProgramView<LiftedTag> program);

ProgramAnalysis analyze_program(formalism::datalog::ProgramView<LiftedTag> program);

TaskVariableDomains compute_variable_domains(formalism::planning::TaskView task);

ProgramVariableDomainsView compute_variable_domain_views(const ProgramVariableDomains& domains, const formalism::datalog::Repository& repository);

TaskVariableDomainsView compute_variable_domain_views(const TaskVariableDomains& domains, const formalism::planning::Repository& repository);

template<typename Callback>
void for_each_compatible_extension(const ConditionalEffectDomainData& domains,
                                   std::span<const ygg::Index<formalism::Object>> prefix,
                                   CompatibilityWorkspace& workspace,
                                   Callback&& callback)
{
    const auto& graph = domains.compatibility_graph;
    const auto& layout = graph.get_layout();
    assert(prefix.size() <= layout.num_partitions);

    workspace.vertex_prefix.resize(prefix.size());
    for (ygg::uint_t partition = 0; partition < prefix.size(); ++partition)
    {
        const auto object = ygg::uint_t(prefix[partition]);
        if (partition >= domains.object_to_vertex.size() || object >= domains.object_to_vertex[partition].size())
            return;

        const auto vertex = domains.object_to_vertex[partition][object];
        if (vertex.index >= layout.num_vertices)
            return;
        workspace.vertex_prefix[partition] = vertex;
    }

    kckp::KCKP(graph).for_each_compatible_extension(workspace.vertex_prefix,
                                                    workspace.kckp,
                                                    [&](std::span<const kckp::Vertex> extension)
                                                    {
                                                        callback(extension
                                                                 | std::views::transform(
                                                                     [&](kckp::Vertex vertex)
                                                                     {
                                                                         const auto partition = layout.vertex_to_partition[vertex.index];
                                                                         const auto bit = layout.vertex_to_bit[vertex.index];
                                                                         return domains.condition_domain.payload[partition].objects[bit];
                                                                     }));
                                                    });
}
}

#endif
