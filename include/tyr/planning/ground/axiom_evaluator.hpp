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

#ifndef TYR_PLANNING_GROUND_AXIOM_EVALUATOR_HPP_
#define TYR_PLANNING_GROUND_AXIOM_EVALUATOR_HPP_

#include "tyr/planning/ground/state_builder.hpp"
//
#include "tyr/formalism/planning/ground_axiom_index.hpp"
#include "tyr/planning/axiom_evaluator.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/match_tree/declarations.hpp"
#include "tyr/planning/ground/match_tree/match_tree.hpp"

#include <vector>

namespace tyr::planning
{
template<>
class AxiomEvaluator<GroundTag>
{
    friend class AxiomEvaluatorFactory<GroundTag>;

private:
    AxiomEvaluator(ygg::uint_t index, TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context);

public:
    AxiomEvaluator(const AxiomEvaluator&) = delete;
    AxiomEvaluator& operator=(const AxiomEvaluator&) = delete;

    void compute_extended_state(UnpackedState<GroundTag>& unpacked_state);

    auto get_index() const noexcept { return m_index; }

private:
    ygg::uint_t m_index;
    TaskPtr<GroundTag> m_task;
    std::vector<match_tree::MatchTreePtr<::tyr::formalism::planning::GroundAxiom>> m_axiom_match_tree_strata;

    ::tyr::formalism::planning::GroundAxiomViewList m_applicable_axioms;
};
}

#endif
