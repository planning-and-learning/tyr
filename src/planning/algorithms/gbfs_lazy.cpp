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

#include "tyr/planning/algorithms/gbfs_lazy.hpp"

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/algorithms/concepts.hpp"
#include "tyr/planning/algorithms/gbfs_lazy/event_handler.hpp"
#include "tyr/planning/algorithms/openlists/alternating.hpp"
#include "tyr/planning/algorithms/strategies/goal.hpp"
#include "tyr/planning/algorithms/strategies/pruning.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/ground/successor_generator.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/heuristic.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/lifted/successor_generator.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/search_space/sequential.hpp"
#include "tyr/planning/state_index.hpp"
#include "tyr/planning/state_routing/single_worker.hpp"
#include "tyr/planning/worker_state_index.hpp"

#include <algorithm>
#include <random>
#include <yggdrasil/containers/segmented_vector.hpp>
#include <yggdrasil/core/chrono.hpp>
#include <yggdrasil/core/portable_shuffle.hpp>

namespace tyr::planning::gbfs_lazy
{

/**
 * GBFS search node
 */

template<TaskKind Kind>
struct SearchNode
{
    ygg::float_t g_value;
    ygg::Index<State<Kind>> parent_state;
    SearchNodeStatus status;
    bool preferred;
};

static_assert(sizeof(SearchNode<LiftedTag>) == 16);
static_assert(sizeof(SearchNode<GroundTag>) == 16);

template<TaskKind Kind>
using SearchNodeVector = ygg::SegmentedVector<SearchNode<Kind>>;

template<TaskKind Kind>
struct SuccessorMetadata
{
    WorkerStateIndex<Kind> parent;
    ygg::float_t source_g_value;
    ygg::float_t inherited_h_value;
    bool preferred;
};

template<TaskKind Kind>
static SearchNode<Kind>& get_or_create_search_node(ygg::Index<State<Kind>> state_index, SearchNodeVector<Kind>& search_nodes)
{
    static auto default_node = SearchNode { std::numeric_limits<ygg::float_t>::infinity(), ygg::Index<State<Kind>>::max(), SearchNodeStatus::NEW, false };

    while (ygg::uint_t(state_index) >= search_nodes.size())
    {
        search_nodes.push_back(default_node);
    }
    return search_nodes[ygg::uint_t(state_index)];
}

/**
 * GBFS queue
 */

template<TaskKind Kind>
struct QueueEntry
{
    using KeyType = std::tuple<ygg::float_t, ygg::float_t, ygg::uint_t>;
    using ItemType = ygg::Index<State<Kind>>;

    ygg::float_t g_value;
    ygg::float_t h_value;
    ygg::Index<State<Kind>> state;
    ygg::uint_t step;

    KeyType get_key() const { return std::make_tuple(h_value, g_value, step); }
    ItemType get_item() const { return state; }
};

static_assert(sizeof(QueueEntry<LiftedTag>) == 24);
static_assert(sizeof(QueueEntry<GroundTag>) == 24);

template<TaskKind Kind>
using Queue = PriorityQueue<QueueEntry<Kind>>;

template<TaskKind Kind>
SearchResult<Kind> find_solution(Task<Kind>& task, SuccessorGenerator<Kind>& successor_generator, Heuristic<Kind>& heuristic, const Options<Kind>& options)
{
    const auto start_node = (options.start_node) ? options.start_node.value() : successor_generator.get_initial_node();
    const auto& start_state = start_node.get_state();
    const auto start_state_index = start_state.get_index();
    const auto event_handler = (options.event_handler) ? options.event_handler : DefaultEventHandler<Kind>::create(0);
    const auto pruning_strategy = (options.pruning_strategy) ? options.pruning_strategy : PruningStrategy<Kind>::create();
    const auto goal_strategy = (options.goal_strategy) ? options.goal_strategy : ConjunctiveGoalStrategy<Kind>::create(task);
    auto rng = std::mt19937_64(options.random_seed);
    auto& state_repository = *successor_generator.get_state_repository();

    auto step = ygg::uint_t(0);
    auto result = SearchResult<Kind>();
    auto search_nodes = SearchNodeVector<Kind>();
    auto preferred_openlist = Queue<Kind>();
    auto standard_openlist = Queue<Kind>();
    auto openlist = AlternatingOpenList<Queue<Kind>, Queue<Kind>>(preferred_openlist, standard_openlist, std::array<size_t, 2> { 1, 1 });
    const auto start_g_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(start_node.get_metric());
    const auto start_h_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(heuristic.evaluate(start_state));
    auto best_h_value = start_h_value;
    const auto start_preferred = false;
    auto& start_search_node = get_or_create_search_node(start_state_index, search_nodes);
    start_search_node.status = (start_h_value == std::numeric_limits<ygg::float_t>::infinity()) ? SearchNodeStatus::DEAD_END : SearchNodeStatus::OPEN;
    start_search_node.g_value = start_g_value;
    start_search_node.preferred = start_preferred;

    event_handler->on_start_search(start_node, start_h_value);

    /* Test static goal. */

    if (!goal_strategy->is_static_goal_satisfied(task))
    {
        result.status = SearchStatus::UNSOLVABLE;
        event_handler->on_end_search(result.status);
        return result;
    }

    /* Test whether initial state is goal. */

    if (goal_strategy->is_dynamic_goal_satisfied(start_state, start_state))
    {
        result.plan = Plan(start_node, LabeledNodeList<Kind> {});
        result.goal_node = start_node;
        result.status = SearchStatus::SOLVED;

        event_handler->on_end_search(result.status);
        event_handler->on_solved(result.plan.value());

        return result;
    }

    if (std::isnan(start_node.get_metric()))
    {
        event_handler->on_end_search(SearchStatus::FAILED);

        throw std::runtime_error("find_solution(...): start node metric value is NaN.");
    }

    /* Test whether start state is deadend. */

    if (start_search_node.status == SearchNodeStatus::DEAD_END)
    {
        result.status = SearchStatus::UNSOLVABLE;
        event_handler->on_end_search(result.status);
        return result;
    }

    /* Test whether initial state should be pruned. */

    if (pruning_strategy->should_prune_state(start_state))
    {
        result.status = SearchStatus::EXHAUSTED;
        event_handler->on_end_search(result.status);
        return result;
    }

    auto applicable_actions = std::vector<::tyr::formalism::planning::ActionBindingView> {};
    auto routed_successors = std::vector<RoutedSuccessor<Kind, SuccessorMetadata<Kind>>> {};
    auto state_router = SingleWorkerStateRouter<Kind, RandomDistHashTag, SuccessorMetadata<Kind>>(options.random_seed);

    standard_openlist.insert(QueueEntry { start_g_value, start_h_value, start_state_index, step++ });

    auto stopwatch = options.max_time ? std::optional<ygg::CountdownWatch>(options.max_time.value()) : std::nullopt;

    auto& openlist_weights = openlist.get_weights();

    while (!openlist.empty())
    {
        if (stopwatch && stopwatch->has_finished())
        {
            result.status = SearchStatus::OUT_OF_TIME;
            event_handler->on_end_search(result.status);
            return result;
        }

        const auto state_index = openlist.top();
        const auto state = state_repository.get_registered_state(state_index);

        openlist.pop();
        // Weight decay of prefered queue
        openlist_weights[0] = std::max(openlist_weights[0] - 1, size_t { 1 });

        auto& search_node = get_or_create_search_node(state_index, search_nodes);
        auto node = Node<Kind>(state, search_node.g_value);

        /* Close state. */

        if (search_node.status == SearchNodeStatus::CLOSED || search_node.status == SearchNodeStatus::DEAD_END)
        {
            continue;
        }

        /* Expand the successors of the node. */

        event_handler->on_expand_node(node);

        const auto state_h_value = ygg::FloatTolerance<ygg::float_t>::canonicalize(heuristic.evaluate(state));
        if (state_h_value == std::numeric_limits<ygg::float_t>::infinity())
        {
            search_node.status = SearchNodeStatus::DEAD_END;
            continue;
        }

        if (state_h_value < best_h_value)
        {
            best_h_value = state_h_value;
            event_handler->on_new_best_h_value(best_h_value);

            // Boost prefered queue
            if (options.use_preferred_actions)
                openlist_weights[0] += options.boost_preferred_queue;
        }

        const auto* preferred_actions = options.use_preferred_actions ? &heuristic.get_preferred_actions() : nullptr;

        /* Ensure that the state is closed */

        search_node.status = SearchNodeStatus::CLOSED;

        successor_generator.get_applicable_action_bindings(node, applicable_actions);
        routed_successors.clear();
        routed_successors.reserve(applicable_actions.size());
        for (const auto action : applicable_actions)
        {
            auto successor_state = state_repository.get_state_builder();
            const auto action_result = successor_generator.generate_successor_state(node, action, *successor_state);
            const auto preferred = preferred_actions && preferred_actions->contains(action);
            state_router.send(
                std::move(successor_state),
                action_result,
                action,
                SuccessorMetadata<Kind> { WorkerStateIndex<Kind> { ygg::Index<Worker>(0), state_index }, search_node.g_value, state_h_value, preferred });
            routed_successors.push_back(state_router.receive(successor_generator));
        }

        if (options.shuffle_labeled_succ_nodes)
            ygg::portable_shuffle(routed_successors.begin(), routed_successors.end(), rng);

        for (const auto& routed_successor : routed_successors)
        {
            const auto& labeled_succ_node = routed_successor.labeled_node;
            const auto& succ_node = labeled_succ_node.node;
            const auto& succ_state = succ_node.get_state();
            const auto succ_state_index = succ_state.get_index();

            auto& successor_search_node = get_or_create_search_node(succ_state_index, search_nodes);

            assert(!std::isnan(succ_node.get_metric()));

            const auto is_preferred = routed_successor.metadata.preferred;
            const auto is_new_successor_state = (successor_search_node.status == SearchNodeStatus::NEW);

            if (is_new_successor_state && search_nodes.size() >= options.max_num_states)
            {
                result.status = SearchStatus::OUT_OF_STATES;
                event_handler->on_end_search(result.status);
                return result;
            }

            /* Skip previously generated state. */

            if (!is_new_successor_state)
                continue;

            /* Open new state. */

            const auto successor_g_value = compute_successor_g_value(routed_successor.metadata.source_g_value, succ_node.get_metric(), options.cost_mode);
            const auto normalized_succ_node = Node<Kind>(succ_state, successor_g_value);
            const auto normalized_labeled_succ_node = LabeledNode<Kind> { labeled_succ_node.label, normalized_succ_node };

            successor_search_node.status = SearchNodeStatus::OPEN;
            successor_search_node.parent_state = routed_successor.metadata.parent.state;
            successor_search_node.g_value = successor_g_value;
            successor_search_node.preferred = is_preferred;

            /* Early goal test. */

            const auto successor_is_goal_state = goal_strategy->is_dynamic_goal_satisfied(start_state, succ_state);

            if (successor_is_goal_state)
            {
                successor_search_node.status = SearchNodeStatus::GOAL;

                event_handler->on_expand_goal_node(normalized_succ_node);

                result.plan = PlanReconstructionPolicy<SequentialSearch>::extract_total_ordered_plan(successor_search_node,
                                                                                                     normalized_succ_node,
                                                                                                     search_nodes,
                                                                                                     successor_generator,
                                                                                                     options.cost_mode);
                result.goal_node = normalized_succ_node;
                result.status = SearchStatus::SOLVED;

                event_handler->on_end_search(result.status);
                event_handler->on_solved(result.plan.value());

                return result;
            }

            /* Apply pruning strategy */

            if (pruning_strategy->should_prune_successor_state(state, succ_state, is_new_successor_state))
            {
                successor_search_node.status = SearchNodeStatus::CLOSED;
                event_handler->on_prune_node(node, normalized_labeled_succ_node);
                continue;
            }

            event_handler->on_generate_node(node, normalized_labeled_succ_node);

            /* Exploration strategy */

            if (is_preferred)
                preferred_openlist.insert(QueueEntry { successor_g_value, routed_successor.metadata.inherited_h_value, succ_state_index, step++ });
            else
                standard_openlist.insert(QueueEntry { successor_g_value, routed_successor.metadata.inherited_h_value, succ_state_index, step++ });
        }
    }

    result.status = SearchStatus::EXHAUSTED;
    event_handler->on_end_search(result.status);
    return result;
}

template SearchResult<LiftedTag> find_solution<LiftedTag>(Task<LiftedTag>& task,
                                                          SuccessorGenerator<LiftedTag>& successor_generator,
                                                          Heuristic<LiftedTag>& heuristic,
                                                          const Options<LiftedTag>& options);

template SearchResult<GroundTag> find_solution<GroundTag>(Task<GroundTag>& task,
                                                          SuccessorGenerator<GroundTag>& successor_generator,
                                                          Heuristic<GroundTag>& heuristic,
                                                          const Options<GroundTag>& options);

static_assert(SolverConcept<Solver<LiftedTag>, LiftedTag>);
static_assert(SolverConcept<Solver<GroundTag>, GroundTag>);

}
