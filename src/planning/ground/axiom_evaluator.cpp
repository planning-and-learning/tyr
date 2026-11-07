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

#include "../axiom_evaluator_impl.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "yggdrasil/formalism/binding_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/ground/axiom_stratification.hpp"
#include "tyr/planning/ground/match_tree/match_tree.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/state_builder.hpp"

#include <atomic>
#include <memory>
#include <urd/elements/less.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

namespace detail
{
template<>
struct AxiomDefinition<GroundTag>
{
    explicit AxiomDefinition(TaskPtr<GroundTag> task_) : task(std::move(task_)), match_tree_prototypes()
    {
        auto axiom_strata = compute_ground_axiom_stratification(task->get_task());
        match_tree_prototypes.reserve(axiom_strata.data.size());
        for (const auto& stratum : axiom_strata.data)
            match_tree_prototypes.emplace_back(match_tree::MatchTree<formalism::planning::Axiom<GroundTag>>::create(stratum, task->get_task().context()));
    }

    TaskPtr<GroundTag> task;
    urd::Store<urd::sequence<match_tree::MatchTreePtr<formalism::planning::Axiom<GroundTag>>>> match_tree_prototypes;
};

template<>
struct AxiomWorkspace<GroundTag>
{
    AxiomWorkspace(const AxiomDefinition<GroundTag>& definition, ygg::ExecutionContextPtr execution_context_) :
        execution_context(std::move(execution_context_)),
        match_tree_workers(),
        applicable_axioms()
    {
        match_tree_workers.reserve(definition.match_tree_prototypes.size());
        for (const auto& prototype : definition.match_tree_prototypes)
            match_tree_workers.push_back(prototype->make_worker());
    }

    ygg::ExecutionContextPtr execution_context;
    urd::Store<urd::sequence<match_tree::MatchTreePtr<formalism::planning::Axiom<GroundTag>>>> match_tree_workers;
    formalism::planning::AxiomViewList<GroundTag> applicable_axioms;
};
}

template<>
void AxiomEvaluator<GroundTag>::compute_extended_state(urd::builder<State<GroundTag>>& state_builder)
{
    auto state_context = StateContext<GroundTag> { *m_impl->definition->task, state_builder, static_cast<ygg::float_t>(0) };

    for (const auto& match_tree : m_impl->evaluator.match_tree_workers)
    {
        while (true)
        {
            auto discovered_new_atom = false;

            m_impl->evaluator.applicable_axioms.clear();
            match_tree->generate(state_context, m_impl->evaluator.applicable_axioms);

            for (const auto axiom : m_impl->evaluator.applicable_axioms)
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

template class AxiomEvaluator<GroundTag>;

}
