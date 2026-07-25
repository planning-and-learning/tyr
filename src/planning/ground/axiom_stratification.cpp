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

#include "tyr/planning/ground/axiom_stratification.hpp"

#include "../../analysis/stratification_utils.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;
namespace as = tyr::analysis::stratification;

namespace tyr::planning
{

// Build dependency graph: nodes = derived ground atoms
static as::DepGraph build_dependency_graph(fp::FDRTaskView task, size_t num_atoms)
{
    as::DepGraph graph(num_atoms);

    for (const auto axiom : task.get_ground_axioms())
    {
        const auto h_atom = axiom.get_head().get_index();

        for (const auto literal : axiom.get_body().get_literals<f::DerivedTag>())
        {
            const auto b_atom = literal.get_atom().get_index();

            as::EdgeProps ep;
            ep.kind = literal.get_polarity() ? as::EdgeKind::NonStrict : as::EdgeKind::Strict;
            boost::add_edge(b_atom.value, h_atom.value, ep, graph);
        }
    }

    return graph;
}

GroundAxiomStrata compute_ground_axiom_stratification(fp::FDRTaskView task)
{
    const auto num_atoms = task.get_atoms<f::DerivedTag>().size();

    // 1) dependency graph
    const auto dep = build_dependency_graph(task, num_atoms);

    // 2) SCC + check
    const auto [comp, num_comps] = as::compute_scc_and_check(dep);

    // 3) Condensed DAG
    const auto dag = as::build_condensation_dag(dep, comp, num_comps);

    // 4) Component strata
    const auto comp_stratum = as::compute_component_strata(dag);

    // 5) Assign each derived atom a stratum
    auto atom_stratum = std::vector<ygg::uint_t>(num_atoms, 0);
    for (ygg::uint_t i = 0; i < num_atoms; ++i)
        atom_stratum[i] = comp_stratum[comp[i]];

    // 6) Bucket axioms by head stratum
    auto max_s = ygg::uint_t(0);
    for (auto s : comp_stratum)
        max_s = std::max(max_s, s);

    auto buckets = std::vector<GroundAxiomStratum>(max_s + 1);
    for (const auto axiom : task.get_ground_axioms())
        buckets[atom_stratum[ygg::uint_t(axiom.get_head().get_index())]].push_back(axiom);

    auto out = GroundAxiomStrata {};
    out.data.reserve(buckets.size());
    for (auto& b : buckets)
        out.data.emplace_back(GroundAxiomStratum(std::move(b)));

    return out;
}
}
