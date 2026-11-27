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

#ifndef TYR_PLANNING_ALGORITHMS_SERIALIZED_HPP_
#define TYR_PLANNING_ALGORITHMS_SERIALIZED_HPP_

#include "tyr/planning/nodes/node_concepts.hpp"
#include "tyr/planning/algorithms/concepts.hpp"
#include "tyr/planning/algorithms/serialized/event_handler.hpp"
#include "tyr/planning/algorithms/serialized/statistics.hpp"
#include "tyr/planning/algorithms/serialized_concepts.hpp"
#include "tyr/planning/algorithms/strategies/goal.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/plans/plan_view.hpp"
#include "tyr/planning/successors/successors.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace tyr::planning::serialized
{

template<TaskKind Kind, std::copy_constructible Stats = tyr::planning::Statistics>
struct Options
{
    /// Optional initial node for the first subsearch; when a subsearch runs, it must belong to the subsolver's task.
    std::optional<OwnedNodeRecordView<Kind>> start_node = std::nullopt;
    EventHandlerPtr<Kind, Stats> event_handler = nullptr;
    GoalStrategyPtr<Kind> subgoal_strategy = nullptr;
    GoalStrategyPtr<Kind> goal_strategy = nullptr;
    ygg::uint_t max_num_subsearches = std::numeric_limits<ygg::uint_t>::max();
    /// Total wall-clock budget shared by all subsearches.
    std::optional<std::chrono::steady_clock::duration> max_time = std::nullopt;

    Options() = default;
};

namespace detail
{

/// A packed node's record (its state's key and its metric).
template<TaskKind Kind, typename V>
urd::data<Node<Kind>> record_of(const V& node)
{ return urd::data<Node<Kind>>(node.get_state().index(), node.get_metric()); }

/// A node of a plan kept apart from it, packed.
template<TaskKind Kind>
OwnedNodeRecordView<Kind> kept(const NodeBuilderView<Kind>& node)
{ return OwnedNodeRecordView<Kind>(node.pack(), &node.context()); }

/// The plan from `start` through `steps` (records of `repository`), unpacked from `repository`.
template<TaskKind Kind>
OwnedPlanBuilderView<Kind> make_plan(const StateRepository<Kind>& repository, const urd::data<Node<Kind>>& start, const LabeledNodeRecordList<Kind>& steps)
{
    auto plan = PlanBuilder<Kind>();
    plan.start_node = get_node(repository, start);
    plan.labeled_succ_nodes.reserve(steps.size());
    for (const auto& step : steps)
        plan.labeled_succ_nodes.push_back(get_labeled_node(repository, step));
    return OwnedPlanBuilderView<Kind>(std::move(plan), &repository);
}

/// The node a plan ends in, packed.
template<TaskKind Kind>
OwnedNodeRecordView<Kind> goal_of(const OwnedPlanBuilderView<Kind>& plan)
{ return OwnedNodeRecordView<Kind>(plan.get_goal_node().pack(), &plan.context()); }

template<TaskKind Kind>
SearchResult<Kind> make_empty_result(const OwnedNodeRecordView<Kind>& start_node, SearchStatus status)
{
    auto result = SearchResult<Kind> {};
    result.status = status;
    result.goal_node = start_node;
    result.plan.emplace(make_plan(start_node.context(), record_of<Kind>(start_node), LabeledNodeRecordList<Kind> {}));
    return result;
}

template<TaskKind Kind, SolverConcept<Kind> Solver>
void append_plan(Solver& solver, const PlanBuilderView<Kind>& subplan, OwnedNodeRecordView<Kind>& current_node, LabeledNodeRecordList<Kind>& labeled_succ_nodes)
{
    auto previous_metric = solver.normalize_start_node(kept(subplan.get_start_node())).get_metric();
    auto cumulative_metric = current_node.get_metric();

    for (const auto& labeled_succ_node : subplan.get_labeled_succ_nodes())
    {
        const auto normalized_succ_node = solver.normalize_start_node(kept(labeled_succ_node.get_node()));
        const auto step_cost = normalized_succ_node.get_metric() - previous_metric;
        cumulative_metric = ygg::FloatTolerance<ygg::float_t>::canonicalize(cumulative_metric + step_cost);
        if (!std::isfinite(cumulative_metric))
            throw std::runtime_error("serialized::find_solution(...): cumulative plan metric is not finite.");
        current_node = OwnedNodeRecordView<Kind>(urd::data<Node<Kind>>(normalized_succ_node.get_state().index(), cumulative_metric), &normalized_succ_node.context());
        labeled_succ_nodes.emplace_back(urd::interned_view(labeled_succ_node.get_label()).index(), record_of<Kind>(current_node));
        previous_metric = normalized_succ_node.get_metric();
    }
}

template<TaskKind Kind>
struct ReachedSubgoal
{
    OwnedNodeRecordView<Kind> node;
    size_t plan_position;
};

template<TaskKind Kind>
std::optional<size_t> find_reached_subgoal(const urd::Store<urd::sequence<ReachedSubgoal<Kind>>>& reached_subgoals, const OwnedNodeRecordView<Kind>& node)
{
    for (const auto& reached_subgoal : reached_subgoals)
    {
        if (reached_subgoal.node.get_state() == node.get_state())
            return reached_subgoal.plan_position;
    }

    return std::nullopt;
}

/// Whether the dynamic goal of `strategy` holds in the state of `node`, seeded by `seed` (both registered states, read
/// unpacked).
template<TaskKind Kind>
bool is_dynamic_goal_satisfied(GoalStrategy<Kind>& strategy, const StateBuilderView<Kind>& seed, const OwnedNodeRecordView<Kind>& node)
{
    const auto state = ::tyr::planning::get_registered_state(node.context(), node.get_state().index());
    return strategy.is_dynamic_goal_satisfied(seed, urd::resolve(*state, node.context()));
}

}

template<TaskKind Kind, SerializedSolverConcept<Kind> Solver, std::copy_constructible Stats, typename Reader>
    requires requires(Reader& reader, const Solver& solver, const SearchResult<Kind>& result) {
        { std::invoke(reader, solver, result) } -> std::same_as<const Stats*>;
    }
SearchResult<Kind> find_solution(Solver& solver, const Options<Kind, Stats>& options, Reader read_statistics)
{
    const auto event_handler = options.event_handler ? options.event_handler : DefaultEventHandler<Kind, Stats>::create();

    if (!options.subgoal_strategy)
        throw std::invalid_argument("serialized::find_solution(...): subgoal strategy is required.");
    if (!options.goal_strategy)
        throw std::invalid_argument("serialized::find_solution(...): goal strategy is required.");

    auto statistics = tyr::planning::Statistics {};
    auto worker_statistics = urd::Store<urd::sequence<tyr::planning::Statistics>> {};
    const auto search_start = std::chrono::steady_clock::now();
    const auto max_time =
        options.max_time ? options.max_time : std::optional<std::chrono::steady_clock::duration>(std::as_const(solver).options.search_budget.max_time);
    const auto deadline = max_time ? std::make_optional(search_start + *max_time) : std::optional<std::chrono::steady_clock::time_point> {};
    statistics.set_search_start_time_point(search_start);

    auto current_start_node = solver.normalize_start_node(options.start_node);
    const auto combined_start_node = current_start_node;
    const auto& state_repository = combined_start_node.context();
    const auto final_goal_seed_state = ::tyr::planning::get_registered_state(state_repository, combined_start_node.get_state().index());
    const auto final_goal_seed = urd::resolve(*final_goal_seed_state, state_repository);

    event_handler->on_start_search();

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
        if (result.plan && result.status == SearchStatus::SOLVED)
            event_handler->on_solved(*result.plan);
        event_handler->on_end_search(result.status, result.statistics);
        return result;
    };

    if (!options.goal_strategy->is_static_goal_satisfied(*final_goal_seed.context().get_task()))
    {
        auto result = SearchResult<Kind> {};
        result.status = SearchStatus::UNSOLVABLE;
        return finalize(std::move(result));
    }

    if (options.goal_strategy->is_dynamic_goal_satisfied(final_goal_seed, final_goal_seed))
        return finalize(detail::make_empty_result(combined_start_node, SearchStatus::SOLVED));

    if (options.max_num_subsearches == 0)
        return finalize(detail::make_empty_result(combined_start_node, SearchStatus::EXHAUSTED));

    auto combined_labeled_succ_nodes = LabeledNodeRecordList<Kind> {};
    auto reached_subgoals = urd::Store<urd::sequence<detail::ReachedSubgoal<Kind>>> { { combined_start_node, 0 } };

    for (auto subsearch_index = ygg::uint_t { 0 }; subsearch_index < options.max_num_subsearches; ++subsearch_index)
    {
        const auto subsearch_start = std::chrono::steady_clock::now();
        if (deadline && subsearch_start >= *deadline)
        {
            auto result = SearchResult<Kind> {};
            result.status = SearchStatus::OUT_OF_TIME;
            if (!combined_labeled_succ_nodes.empty())
            {
                result.plan = detail::make_plan(state_repository, detail::record_of<Kind>(combined_start_node), combined_labeled_succ_nodes);
                result.goal_node = detail::goal_of(*result.plan);
            }
            return finalize(std::move(result));
        }

        auto local_solver = solver;
        local_solver.options.start_node = current_start_node;
        local_solver.options.goal_strategy = options.subgoal_strategy;

        event_handler->on_start_subsearch(subsearch_index);

        if (deadline)
            local_solver.options.search_budget.max_time = std::max(*deadline - std::chrono::steady_clock::now(), std::chrono::steady_clock::duration::zero());
        auto sub_result = local_solver.solve();
        statistics.add(sub_result.statistics);
        worker_statistics.resize(std::max(worker_statistics.size(), sub_result.worker_statistics.size()));
        for (size_t i = 0; i < sub_result.worker_statistics.size(); ++i)
            worker_statistics[i].add(sub_result.worker_statistics[i]);

        if (const auto* reported_statistics = std::invoke(read_statistics, std::as_const(local_solver), std::as_const(sub_result)))
            event_handler->add_subsearch_statistics(sub_result.statistics, *reported_statistics);

        event_handler->on_end_subsearch(subsearch_index, sub_result.status);

        if (sub_result.status != SearchStatus::SOLVED)
            return finalize(std::move(sub_result));

        if (!sub_result.plan || !sub_result.goal_node)
        {
            auto result = SearchResult<Kind> {};
            result.status = SearchStatus::FAILED;
            return finalize(std::move(result));
        }

        auto current_node =
            combined_labeled_succ_nodes.empty() ? combined_start_node : OwnedNodeRecordView<Kind>(combined_labeled_succ_nodes.back().node, &state_repository);
        if (sub_result.plan->empty())
        {
            auto result = SearchResult<Kind> {};
            result.status =
                detail::is_dynamic_goal_satisfied(*options.goal_strategy, final_goal_seed, current_node) ? SearchStatus::SOLVED : SearchStatus::EXHAUSTED;
            result.plan = detail::make_plan(state_repository, detail::record_of<Kind>(combined_start_node), combined_labeled_succ_nodes);
            result.goal_node = current_node;

            return finalize(std::move(result));
        }

        detail::append_plan(solver, *sub_result.plan, current_node, combined_labeled_succ_nodes);

        if (detail::is_dynamic_goal_satisfied(*options.goal_strategy, final_goal_seed, current_node))
        {
            auto result = SearchResult<Kind> {};
            result.status = SearchStatus::SOLVED;
            result.plan = detail::make_plan(state_repository, detail::record_of<Kind>(combined_start_node), combined_labeled_succ_nodes);
            result.goal_node = detail::goal_of(*result.plan);

            return finalize(std::move(result));
        }

        if (const auto cycle_begin = detail::find_reached_subgoal(reached_subgoals, current_node))
        {
            auto result = SearchResult<Kind> {};
            result.status = SearchStatus::CYCLE;
            result.plan = detail::make_plan(state_repository, detail::record_of<Kind>(combined_start_node), combined_labeled_succ_nodes);
            result.goal_node = detail::goal_of(*result.plan);
            result.cycle_range = std::pair<size_t, size_t> { *cycle_begin, result.plan->get_length() };

            return finalize(std::move(result));
        }

        current_start_node = OwnedNodeRecordView<Kind>(urd::data<Node<Kind>>(current_node.get_state().index(), 0), &current_node.context());
        reached_subgoals.push_back(detail::ReachedSubgoal<Kind> { current_start_node, combined_labeled_succ_nodes.size() });
    }

    auto result = SearchResult<Kind> {};
    result.status = SearchStatus::EXHAUSTED;
    result.plan = detail::make_plan(state_repository, detail::record_of<Kind>(combined_start_node), combined_labeled_succ_nodes);
    result.goal_node = detail::goal_of(*result.plan);
    return finalize(std::move(result));
}

template<TaskKind Kind, SerializedSolverConcept<Kind> Solver>
SearchResult<Kind> find_solution(Solver& solver, const Options<Kind>& options = {})
{
    return find_solution<Kind>(solver, options, [](const Solver&, const SearchResult<Kind>& result) { return &result.statistics; });
}

template<TaskKind Kind, SerializedSolverConcept<Kind> Subsolver>
struct Solver
{
    Subsolver subsolver;
    Options<Kind> options;

    OwnedNodeRecordView<Kind> normalize_start_node(std::optional<OwnedNodeRecordView<Kind>> start_node)
    {
        if (!start_node)
            start_node = options.start_node;
        return subsolver.normalize_start_node(start_node);
    }

    SearchResult<Kind> solve() { return find_solution<Kind>(subsolver, options); }
};

}  // namespace tyr::planning::serialized

#endif
