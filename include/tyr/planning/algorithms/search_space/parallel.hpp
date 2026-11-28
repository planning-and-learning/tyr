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

#ifndef TYR_PLANNING_ALGORITHMS_SEARCH_SPACE_PARALLEL_HPP_
#define TYR_PLANNING_ALGORITHMS_SEARCH_SPACE_PARALLEL_HPP_

#include "tyr/planning/algorithms/search_space/search_node.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/states/worker_state_index.hpp"
#include "tyr/planning/successors/successors.hpp"

#include <algorithm>
#include <cassert>
#include <span>
#include <stdexcept>
#include <urd/elements/equal_to.hpp>
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::planning
{

/// Borrows the search space of one worker; the referenced objects must outlive the view.
template<TaskKind Kind, SearchNodeConcept<WorkerStateIndex<Kind>> SearchNode>
class WorkerSearchSpaceView
{
public:
    WorkerSearchSpaceView(StateRepository<Kind>& state_repository,
                          AxiomEvaluator<Kind>& axiom_evaluator,
                          SuccessorGenerator<Kind>& successor_generator,
                          const urd::Store<urd::sequence<SearchNode>, urd::stable_refs>& search_nodes,
                          size_t state_index_divisor = 1) :
        m_state_repository(&state_repository),
        m_axiom_evaluator(&axiom_evaluator),
        m_successor_generator(&successor_generator),
        m_search_nodes(&search_nodes),
        m_state_index_divisor(state_index_divisor)
    {
    }

    [[nodiscard]] StateRepository<Kind>& state_repository() const noexcept { return *m_state_repository; }

    [[nodiscard]] AxiomEvaluator<Kind>& axiom_evaluator() const noexcept { return *m_axiom_evaluator; }

    [[nodiscard]] SuccessorGenerator<Kind>& successor_generator() const noexcept { return *m_successor_generator; }

    [[nodiscard]] const urd::Store<urd::sequence<SearchNode>, urd::stable_refs>& search_nodes() const noexcept { return *m_search_nodes; }

    [[nodiscard]] const SearchNode& get_search_node(urd::index<State<Kind>> state) const
    {
        assert(m_state_index_divisor > 0);
        const auto index = static_cast<size_t>(ygg::uint_t(state.value)) / m_state_index_divisor;
        assert(index < m_search_nodes->size());
        return (*m_search_nodes)[index];
    }

private:
    StateRepository<Kind>* m_state_repository;
    AxiomEvaluator<Kind>* m_axiom_evaluator;
    SuccessorGenerator<Kind>* m_successor_generator;
    const urd::Store<urd::sequence<SearchNode>, urd::stable_refs>* m_search_nodes;
    size_t m_state_index_divisor;
};

template<>
struct PlanReconstructionPolicy<ParallelSearch>
{
    template<TaskKind Kind, SearchNodeConcept<WorkerStateIndex<Kind>> SearchNode>
    static PlanBuilder<DeterministicTag, Kind> extract_total_ordered_plan(WorkerStateIndex<Kind> final_state,
                                                                          std::span<const WorkerSearchSpaceView<Kind, SearchNode>> workers,
                                                                          StateRepository<Kind>& caller_state_repository,
                                                                          CostMode action_cost_mode = CostMode::GENERAL)
    {
        auto trajectory = urd::Store<urd::sequence<WorkerStateIndex<Kind>>> {};
        auto current = final_state;

        while (true)
        {
            trajectory.push_back(current);
            const auto& search_node = get_search_node(current, workers);
            assert(search_node.parent_state.worker.is_none() == search_node.parent_state.state.is_none());
            if (search_node.parent_state.is_max())
            {
                break;
            }
            current = search_node.parent_state;
        }
        std::reverse(trajectory.begin(), trajectory.end());

        auto labels = urd::Store<urd::sequence<formalism::ActionRowView>> {};
        labels.reserve(trajectory.size() - 1);
        auto builder = formalism::Builder {};

        for (size_t i = 1; i < trajectory.size(); ++i)
        {
            const auto& source_worker = get_worker(trajectory[i - 1], workers);
            const auto& source_search_node = get_search_node(trajectory[i - 1], workers);
            const auto& target_search_node = get_search_node(trajectory[i], workers);
            const auto target_state = ::tyr::planning::get_registered_state(get_worker(trajectory[i], workers).state_repository(), trajectory[i].state);
            const auto target_view = urd::resolve(*target_state, get_worker(trajectory[i], workers).state_repository());

            // The successor of the source worker's applicable binding that reproduces the transition, its binding interned
            // as the label (the only bindings a search interns).
            auto& generator = source_worker.successor_generator();
            auto& repository = source_worker.state_repository();
            auto source_node = get_node(repository, trajectory[i - 1].state);
            source_node.metric = source_search_node.g_value;
            const auto source_view = urd::resolve(source_node, repository);
            auto found = false;
            for_each_successor<DeterministicTag>(
                generator,
                source_view,
                [&](const TransitionLabel<Kind>& transition, SuccessorState<Kind>& successor_state)
                {
                    find_successor(repository, source_view, successor_state);
                    extend_successor(source_worker.axiom_evaluator(), source_view, successor_state);
                    const auto successor = register_successor(repository, source_view, successor_state);
                    const auto g_value = compute_successor_g_value(source_search_node.g_value, successor.metric, action_cost_mode);
                    if (g_value != target_search_node.g_value || !equal_unextended(urd::resolve(successor, repository).state(), target_view))
                        return true;
                    labels.push_back(intern_action_binding(generator, builder, transition.binding.value()));
                    found = true;
                    return false;
                });
            if (!found)
                throw std::logic_error("Plan reconstruction could not reproduce a worker transition.");
        }

        auto plan = PlanBuilder<DeterministicTag, Kind>();
        plan.start_node = get_node(caller_state_repository, materialize(trajectory.front(), workers, caller_state_repository));
        plan.labeled_succ_nodes.reserve(labels.size());
        for (size_t i = 0; i < labels.size(); ++i)
            plan.labeled_succ_nodes.push_back(
                LabeledNodeBuilder<Kind> { labels[i].index(),
                                           get_node(caller_state_repository, materialize(trajectory[i + 1], workers, caller_state_repository)) });

        return plan;
    }

private:
    template<TaskKind Kind, SearchNodeConcept<WorkerStateIndex<Kind>> SearchNode>
    static const WorkerSearchSpaceView<Kind, SearchNode>& get_worker(WorkerStateIndex<Kind> state,
                                                                     std::span<const WorkerSearchSpaceView<Kind, SearchNode>> workers)
    {
        const auto index = static_cast<size_t>(ygg::uint_t(state.worker.value));
        assert(index < workers.size());
        return workers[index];
    }

    template<TaskKind Kind, SearchNodeConcept<WorkerStateIndex<Kind>> SearchNode>
    static const SearchNode& get_search_node(WorkerStateIndex<Kind> state, std::span<const WorkerSearchSpaceView<Kind, SearchNode>> workers)
    { return get_worker(state, workers).get_search_node(state.state); }

    /// A worker's state as a packed node of the caller's repository: the state registered there, and its g value.
    template<TaskKind Kind, SearchNodeConcept<WorkerStateIndex<Kind>> SearchNode>
    static urd::data<Node<Kind>>
    materialize(WorkerStateIndex<Kind> state, std::span<const WorkerSearchSpaceView<Kind, SearchNode>> workers, StateRepository<Kind>& caller_repository)
    {
        const auto& worker = get_worker(state, workers);
        const auto source_state = ::tyr::planning::get_registered_state(worker.state_repository(), state.state);
        const auto target_state = register_extended_state(caller_repository, urd::resolve(*source_state, worker.state_repository()));
        return urd::data<Node<Kind>>(target_state->index, get_search_node(state, workers).g_value);
    }
};

}

#endif
