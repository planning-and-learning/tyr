/*
 * Copyright (C) 2026 Dominik Drexler
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

#include "tyr/planning/state_routing/dist_hash.hpp"

#include "tyr/formalism/planning/ground_conjunctive_condition_view.hpp"
#include "tyr/planning/ground/heuristics/lmcut.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/heuristics/lmcut.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <algorithm>
#include <stdexcept>
#include <yggdrasil/execution/onetbb.hpp>

namespace f = ::tyr::formalism;

namespace tyr::planning
{

template<TaskKind Kind>
void DistHash<Kind, LMCutDistHashTag>::initialize(const StateView<Kind>& start_state)
{
    if (m_initialized)
        throw std::logic_error("LMCutDistHash::initialize(...): distribution features are already initialized.");

    const auto& task = start_state.get_state_repository()->get_task();
    auto heuristic = LMCutHeuristic<Kind>(task, ygg::ExecutionContext::create(1), CostMode::UNIT);
    auto atoms = heuristic.compute_cut_frontier_atoms(start_state.get_state_builder());

    for (const auto fact : task->get_task().get_goal().template get_facts<f::PositiveTag>())
        if (const auto atom = fact.get_atom())
            atoms.push_back(*atom);

    std::ranges::sort(atoms, {}, [](const auto atom) { return atom.get_index(); });
    atoms.erase(std::ranges::unique(atoms, {}, [](const auto atom) { return atom.get_index(); }).begin(), atoms.end());

    m_features.reserve(atoms.size());
    for (const auto atom : atoms)
        m_features.push_back(Feature { atom.get_index(), task->get_fdr_context()->get_fact(atom).get_data() });

    m_initialized = true;
}

template<TaskKind Kind>
ygg::hash_t DistHash<Kind, LMCutDistHashTag>::hash(const ygg::Builder<State<Kind>>& state) const noexcept
{
    assert(m_initialized);
    if (m_features.empty())
        // No selected propositional features means LM-cut provides no partitioning signal.
        return m_fallback.hash(state);

    auto result = static_cast<ygg::hash_t>(m_seed);
    for (const auto& feature : m_features)
        if (state.get(feature.fact.variable) == feature.fact.value)
            ygg::hash_combine(result, feature.atom);
    return result;
}

template class DistHash<GroundTag, LMCutDistHashTag>;
template class DistHash<LiftedTag, LMCutDistHashTag>;

}
