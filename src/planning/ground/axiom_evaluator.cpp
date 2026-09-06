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

#include <atomic>
#include <memory>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/semantics/comparators.hpp>

namespace fp = tyr::formalism::planning;

namespace tyr::planning
{

struct AxiomEvaluator<GroundTag>::Impl
{
    struct Definition
    {
        explicit Definition(TaskPtr<GroundTag> task_) : task(std::move(task_)), match_tree_prototypes()
        {
            auto axiom_strata = compute_ground_axiom_stratification(task->get_task());
            match_tree_prototypes.reserve(axiom_strata.data.size());
            for (const auto& stratum : axiom_strata.data)
                match_tree_prototypes.emplace_back(match_tree::MatchTree<fp::Axiom<::tyr::GroundTag>>::create(stratum, task->get_task().get_context()));
        }

        TaskPtr<GroundTag> task;
        std::vector<match_tree::MatchTreePtr<fp::Axiom<::tyr::GroundTag>>> match_tree_prototypes;
    };

    struct Evaluator
    {
        Evaluator(const Definition& definition, ygg::ExecutionContextPtr execution_context_) :
            execution_context(std::move(execution_context_)),
            match_tree_workers(),
            applicable_axioms()
        {
            match_tree_workers.reserve(definition.match_tree_prototypes.size());
            for (const auto& prototype : definition.match_tree_prototypes)
                match_tree_workers.push_back(prototype->make_worker());
        }

        ygg::ExecutionContextPtr execution_context;
        std::vector<match_tree::MatchTreePtr<fp::Axiom<::tyr::GroundTag>>> match_tree_workers;
        fp::AxiomViewList<::tyr::GroundTag> applicable_axioms;
    };

    Impl(ygg::uint_t index_, TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context_, std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::make_shared<Definition>(std::move(task))),
        evaluator(*definition, std::move(execution_context_))
    {
    }

    Impl(ygg::uint_t index_,
         std::shared_ptr<const Definition> definition_,
         ygg::ExecutionContextPtr execution_context_,
         std::shared_ptr<std::atomic<ygg::uint_t>> next_index_) :
        index(index_),
        next_index(std::move(next_index_)),
        definition(std::move(definition_)),
        evaluator(*definition, std::move(execution_context_))
    {
    }

    ygg::uint_t index;
    std::shared_ptr<std::atomic<ygg::uint_t>> next_index;
    std::shared_ptr<const Definition> definition;
    Evaluator evaluator;
};

AxiomEvaluator<GroundTag>::~AxiomEvaluator() = default;

AxiomEvaluator<GroundTag>::AxiomEvaluator(ygg::uint_t index,
                                          TaskPtr<GroundTag> task,
                                          ygg::ExecutionContextPtr execution_context,
                                          std::shared_ptr<std::atomic<ygg::uint_t>> next_index) :
    m_impl(std::make_unique<Impl>(index, std::move(task), std::move(execution_context), std::move(next_index)))
{
}

AxiomEvaluator<GroundTag>::AxiomEvaluator(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

AxiomEvaluatorPtr<GroundTag> AxiomEvaluator<GroundTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    return AxiomEvaluatorPtr<GroundTag>(new AxiomEvaluator<GroundTag>(std::make_unique<Impl>(m_impl->next_index->fetch_add(1, std::memory_order_relaxed),
                                                                                             m_impl->definition,
                                                                                             std::move(execution_context),
                                                                                             m_impl->next_index)));
}

void AxiomEvaluator<GroundTag>::compute_extended_state(ygg::Builder<State<GroundTag>>& state_builder)
{
    auto state_context = StateContext<GroundTag> { *m_impl->definition->task, state_builder, ygg::float_t(0) };

    for (const auto& match_tree : m_impl->evaluator.match_tree_workers)
    {
        while (true)
        {
            auto discovered_new_atom = bool { false };

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

const TaskPtr<GroundTag>& AxiomEvaluator<GroundTag>::get_task() const noexcept { return m_impl->definition->task; }

const ygg::ExecutionContextPtr& AxiomEvaluator<GroundTag>::get_execution_context() const noexcept { return m_impl->evaluator.execution_context; }

ygg::uint_t AxiomEvaluator<GroundTag>::get_index() const noexcept { return m_impl->index; }

static_assert(AxiomEvaluatorConcept<AxiomEvaluator<GroundTag>, GroundTag>);

}
