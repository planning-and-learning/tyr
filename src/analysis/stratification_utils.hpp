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

#ifndef TYR_SRC_ANALYSIS_STRATIFICATION_UTILS_HPP_
#define TYR_SRC_ANALYSIS_STRATIFICATION_UTILS_HPP_

#include <algorithm>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/topological_sort.hpp>
#include <gtl/phmap.hpp>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::analysis::stratification
{
enum class EdgeKind : uint8_t
{
    NonStrict = 0,
    Strict = 1
};

struct EdgeProps
{
    EdgeKind kind = EdgeKind::NonStrict;
};

// Graph of derived atom dependencies
using DepGraph = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, EdgeProps>;

using DepV = boost::graph_traits<DepGraph>::vertex_descriptor;
using DepE = boost::graph_traits<DepGraph>::edge_descriptor;

// Directed acyclic graph of strongly connected components of the dependency graph
using Dag = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, boost::no_property, EdgeProps>;

using DagV = boost::graph_traits<Dag>::vertex_descriptor;
using DagE = boost::graph_traits<Dag>::edge_descriptor;

// Run SCCs, check stratifiability, return component id per vertex and #components
inline std::pair<std::vector<ygg::uint_t>, ygg::uint_t> compute_scc_and_check(const DepGraph& g)
{
    auto comp = std::vector<ygg::uint_t>(boost::num_vertices(g), std::numeric_limits<ygg::uint_t>::max());
    const auto num = boost::strong_components(g, boost::make_iterator_property_map(comp.begin(), boost::get(boost::vertex_index, g)));

    // Stratifiable iff no STRICT edge stays within the same SCC
    for (auto e_it = boost::edges(g); e_it.first != e_it.second; ++e_it.first)
    {
        DepE e = *e_it.first;
        if (g[e].kind != EdgeKind::Strict)
            continue;

        const auto u = boost::source(e, g);
        const auto v = boost::target(e, g);
        if (comp[u] == comp[v])
            throw std::runtime_error("Set of ground axioms is not stratifiable (negative cycle within SCC).");
    }

    return { comp, num };
}

// Build condensed DAG of SCCs, with edge weight inc=1 if any strict edge exists between SCCs
inline Dag build_condensation_dag(const DepGraph& g, const std::vector<ygg::uint_t>& comp, ygg::uint_t num_comps)
{
    Dag dag(num_comps);

    using EdgeMap = gtl::flat_hash_map<std::pair<ygg::uint_t, ygg::uint_t>,
                                       EdgeKind,
                                       ygg::Hash<std::pair<ygg::uint_t, ygg::uint_t>>,
                                       ygg::EqualTo<std::pair<ygg::uint_t, ygg::uint_t>>>;
    auto best = EdgeMap {};
    best.reserve(boost::num_edges(g));

    for (auto e_it = boost::edges(g); e_it.first != e_it.second; ++e_it.first)
    {
        const auto e = *e_it.first;
        const auto u = boost::source(e, g);
        const auto v = boost::target(e, g);

        const auto cu = comp[u];
        const auto cv = comp[v];
        if (cu == cv)
            continue;

        const auto k = std::make_pair(cu, cv);
        auto& slot = best[k];
        if (slot != EdgeKind::Strict && g[e].kind == EdgeKind::Strict)
            slot = EdgeKind::Strict;
    }

    for (const auto& [k, inc] : best)
    {
        const auto cu = k.first;
        const auto cv = k.second;
        EdgeProps ep;
        ep.kind = inc;
        boost::add_edge(cu, cv, ep, dag);
    }

    return dag;
}

// Compute minimal strata on SCC DAG: s[dst] = max(s[dst], s[src] + inc)
inline std::vector<ygg::uint_t> compute_component_strata(const Dag& dag)
{
    auto topo = std::vector<DagV> {};
    topo.reserve(boost::num_vertices(dag));
    boost::topological_sort(dag, std::back_inserter(topo));  // reverse topological order

    std::reverse(topo.begin(), topo.end());

    auto s = std::vector<ygg::uint_t>(boost::num_vertices(dag), 0);

    for (const auto u : topo)
    {
        for (auto oe = boost::out_edges(u, dag); oe.first != oe.second; ++oe.first)
        {
            const auto e = *oe.first;
            const auto v = boost::target(e, dag);
            s[v] = std::max(s[v], s[u] + (dag[e].kind == EdgeKind::Strict ? 1u : 0u));
        }
    }
    return s;
}
}

#endif
