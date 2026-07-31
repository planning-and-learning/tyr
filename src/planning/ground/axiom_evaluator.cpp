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

#include "tyr/planning/ground/axiom_evaluator.hpp"

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/ground/axiom_stratification.hpp"
#include "tyr/planning/ground/match_tree/match_tree.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/task.hpp"

#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparators.hpp>

namespace fp = tyr::formalism::planning;

namespace tyr::planning
{

AxiomEvaluator<GroundTag>::AxiomEvaluator(ygg::uint_t index, TaskPtr<GroundTag> task, ygg::ExecutionContextPtr) :
    m_index(index),
    m_task(task),
    m_axiom_match_tree_strata(),
    m_applicable_axioms()
{
    auto axiom_strata = compute_ground_axiom_stratification(m_task->get_task());
    for (const auto& stratum : axiom_strata.data)
        m_axiom_match_tree_strata.emplace_back(match_tree::MatchTree<fp::GroundAxiom>::create(stratum, m_task->get_task().get_context()));
}

void AxiomEvaluator<GroundTag>::compute_extended_state(ygg::Builder<State<GroundTag>>& state_builder)
{
    auto state_context = StateContext<GroundTag> { *m_task, state_builder, ygg::float_t(0) };

    for (const auto& match_tree : m_axiom_match_tree_strata)
    {
        while (true)
        {
            auto discovered_new_atom = bool { false };

            m_applicable_axioms.clear();
            match_tree->generate(state_context, m_applicable_axioms);

            for (const auto axiom : m_applicable_axioms)
            {
                const auto atom = axiom.get_head();

                if (!state_builder.test(atom))
                    discovered_new_atom = true;

                state_builder.set(atom);
            }

            if (!discovered_new_atom)
                break;
        }
    }
}

static_assert(AxiomEvaluatorConcept<AxiomEvaluator<GroundTag>, GroundTag>);

}
