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

#include "tyr/planning/algorithms/iw.hpp"

#include "tyr/planning/algorithms/brfs.hpp"
#include "tyr/planning/algorithms/brfs/event_handler.hpp"
#include "tyr/planning/algorithms/concepts.hpp"
#include "tyr/planning/algorithms/iw/pruning_strategy.hpp"
#include "tyr/planning/algorithms/strategies/pruning.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <vector>

namespace tyr::planning::iw
{

template<TaskKind Kind>
class CombinedPruningStrategy : public PruningStrategy<Kind>
{
public:
    CombinedPruningStrategy(PruningStrategyPtr<Kind> lhs, PruningStrategyPtr<Kind> rhs) : m_lhs(std::move(lhs)), m_rhs(std::move(rhs)) {}

    bool should_prune_state(const StateView<Kind>& state) override { return m_lhs->should_prune_state(state) || m_rhs->should_prune_state(state); }

    bool should_prune_successor_state(const StateView<Kind>& state, const StateView<Kind>& succ_state, bool is_new_succ) override
    {
        return m_lhs->should_prune_successor_state(state, succ_state, is_new_succ) || m_rhs->should_prune_successor_state(state, succ_state, is_new_succ);
    }

    [[nodiscard]] PruningStrategyPtr<Kind> make_worker(ygg::Index<Worker> index) const override
    {
        auto lhs = m_lhs->make_worker(index);
        auto rhs = m_rhs->make_worker(index);
        if (!lhs || !rhs)
            return nullptr;
        return std::make_shared<CombinedPruningStrategy>(std::move(lhs), std::move(rhs));
    }

private:
    PruningStrategyPtr<Kind> m_lhs;
    PruningStrategyPtr<Kind> m_rhs;
};

template<TaskKind Kind>
PruningStrategyPtr<Kind> make_pruning_strategy(ygg::uint_t arity, PruningStrategyPtr<Kind> base_pruning_strategy)
{
    auto novelty_pruning_strategy = NoveltyPruningStrategy<Kind>::create(arity);
    if (!base_pruning_strategy)
        return novelty_pruning_strategy;

    return std::make_shared<CombinedPruningStrategy<Kind>>(std::move(base_pruning_strategy), std::move(novelty_pruning_strategy));
}

template<TaskKind Kind>
SearchResult<Kind> find_solution(brfs::Solver<Kind>& brfs_solver, ygg::uint_t max_arity, const Options<Kind>& options)
{
    if (!brfs_solver.task)
        throw std::invalid_argument("iw::find_solution(...): BRFS task is required.");
    if (!brfs_solver.state_repository)
        throw std::invalid_argument("iw::find_solution(...): BRFS state repository is required.");
    if (!brfs_solver.axiom_evaluator)
        throw std::invalid_argument("iw::find_solution(...): BRFS axiom evaluator is required.");
    if (!brfs_solver.successor_generator)
        throw std::invalid_argument("iw::find_solution(...): BRFS successor generator is required.");
    if (max_arity > MaxArity)
        throw std::invalid_argument("iw::find_solution(...): max_arity exceeds iw::MaxArity.");

    const auto event_handler = options.event_handler ? options.event_handler : DefaultEventHandler<Kind>::create();

    auto statistics = tyr::planning::Statistics {};
    auto worker_statistics = std::vector<tyr::planning::Statistics> {};
    auto result = SearchResult<Kind> {};
    result.status = SearchStatus::EXHAUSTED;
    const auto search_start = std::chrono::steady_clock::now();
    const auto max_time = options.max_time ? options.max_time : brfs_solver.options.max_time;
    const auto deadline = max_time ? std::make_optional(search_start + *max_time) : std::optional<std::chrono::steady_clock::time_point> {};
    statistics.set_search_start_time_point(search_start);

    const auto finalize = [&](SearchResult<Kind> result)
    {
        const auto search_end = std::chrono::steady_clock::now();
        statistics.set_search_end_time_point(search_end);
        for (auto& worker : worker_statistics)
        {
            worker.set_search_start_time_point(search_start);
            worker.set_search_end_time_point(search_end);
        }
        result.statistics = statistics;
        result.worker_statistics = worker_statistics;
        event_handler->on_end_search(result.status, result.statistics);
        return result;
    };

    event_handler->on_start_search(max_arity);

    for (auto arity = ygg::uint_t { 0 }; arity <= max_arity; ++arity)
    {
        const auto arity_start = std::chrono::steady_clock::now();
        if (arity > 0 && deadline && arity_start >= *deadline)
        {
            result.status = SearchStatus::OUT_OF_TIME;
            return finalize(std::move(result));
        }

        event_handler->on_start_arity(arity);

        auto local_brfs_solver = brfs_solver;
        local_brfs_solver.options.start_node = options.start_node ? options.start_node : brfs_solver.options.start_node;
        local_brfs_solver.options.pruning_strategy = make_pruning_strategy<Kind>(arity, brfs_solver.options.pruning_strategy);
        local_brfs_solver.options.goal_strategy = options.goal_strategy ? options.goal_strategy : brfs_solver.options.goal_strategy;
        local_brfs_solver.options.max_num_states = options.max_num_states;
        local_brfs_solver.options.random_seed = options.random_seed;
        local_brfs_solver.options.shuffle_labeled_succ_nodes = options.shuffle_labeled_succ_nodes;

        local_brfs_solver.options.max_time =
            deadline ? std::make_optional(std::max(*deadline - std::chrono::steady_clock::now(), std::chrono::steady_clock::duration::zero())) :
                       std::optional<std::chrono::steady_clock::duration> {};
        result = local_brfs_solver.solve();
        statistics.add(result.statistics);
        worker_statistics.resize(std::max(worker_statistics.size(), result.worker_statistics.size()));
        for (size_t i = 0; i < result.worker_statistics.size(); ++i)
            worker_statistics[i].add(result.worker_statistics[i]);
        event_handler->on_end_arity(arity, result.status);

        if (result.status == SearchStatus::SOLVED)
        {
            event_handler->on_solved(arity);
            return finalize(std::move(result));
        }
        if (result.status != SearchStatus::EXHAUSTED)
            return finalize(std::move(result));
    }

    return finalize(std::move(result));
}

static_assert(SolverConcept<Solver<LiftedTag>, LiftedTag>);
static_assert(SolverConcept<Solver<GroundTag>, GroundTag>);

template SearchResult<LiftedTag> find_solution(brfs::Solver<LiftedTag>&, ygg::uint_t, const Options<LiftedTag>&);
template SearchResult<GroundTag> find_solution(brfs::Solver<GroundTag>&, ygg::uint_t, const Options<GroundTag>&);

}
