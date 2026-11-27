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

#ifndef TYR_PLANNING_DECLARATIONS_HPP_
#define TYR_PLANNING_DECLARATIONS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <concepts>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <urd/elements/builder.hpp>
#include <urd/elements/data.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/handle.hpp>
#include <urd/elements/index.hpp>
#include <yggdrasil/execution/declarations.hpp>

namespace tyr::planning
{

struct Worker
{
    static constexpr std::string_view name = "Worker";
};

struct SequentialSearch
{
};

struct ParallelSearch
{
};

template<typename T>
concept SearchKind = std::same_as<T, SequentialSearch> || std::same_as<T, ParallelSearch>;

template<SearchKind Search>
struct PlanReconstructionPolicy;

template<TaskKind Kind>
class Task;

/// The tags of a state's storages, values its record holds (state_data.hpp): its fluent facts, its derived atoms and its
/// numeric values. A storage's record holds it packed, its builder unpacked (state_storage.hpp).
template<TaskKind Kind>
struct FactStorage
{
    static constexpr std::string_view name = "FactStorage";
    /// A state's storage: stored in a pool of its state repository (StateStorageKind).
    static constexpr bool state_storage = true;
};

template<TaskKind Kind>
struct AtomStorage
{
    static constexpr std::string_view name = "AtomStorage";
    /// A state's storage: stored in a pool of its state repository (StateStorageKind).
    static constexpr bool state_storage = true;
};

template<TaskKind Kind>
struct NumericStorage
{
    static constexpr std::string_view name = "NumericStorage";
    /// A state's storage: stored in a pool of its state repository (StateStorageKind).
    static constexpr bool state_storage = true;
};

/// A state's agenda: the ends of its running durative actions (temporal planning; empty otherwise).
template<TaskKind Kind>
struct TemporalStorage
{
    static constexpr std::string_view name = "TemporalStorage";
    /// A state's storage: stored in a pool of its state repository (StateStorageKind).
    static constexpr bool state_storage = true;
};

/// A state's storage S packed: a key, its handle in the pool of its component repository (state_storage.hpp).
template<TaskKind Kind>
using TaskPtr = std::shared_ptr<Task<Kind>>;

/// The tag of a search node: its record urd::data<Node<Kind>> holds its state's key and its metric (node_data.hpp); its
/// builder urd::builder<Node<Kind>, StateRepository<Kind>> holds its state built, by its handle (node_builder.hpp).
template<TaskKind Kind>
struct Node
{
    static constexpr std::string_view name = "Node";
};

/// The tag of a node and the action binding that reached it (an interned row of its action): its record holds the label
/// and the node's record, its builder the label and the node's builder (labeled_node_data.hpp, labeled_node_builder.hpp).
template<TaskKind Kind>
struct LabeledNode
{
    static constexpr std::string_view name = "LabeledNode";
};

/// The tag of a plan: a start node and the labeled nodes that follow it. Its record holds their records (plan_data.hpp),
/// its builder their builders: states unpacked, registered or not (plan_builder.hpp).
template<TaskKind Kind>
struct Plan
{
    static constexpr std::string_view name = "Plan";
};

class StaticState;

/// The tag of a state: its record urd::data<State<Kind>> is the state as its repository stores it, its storages packed
/// (state_data.hpp); its builder urd::builder<State<Kind>, StateRepository<Kind>> holds them unpacked (state_builder.hpp).
template<TaskKind Kind>
struct State
{
    static constexpr std::string_view name = "State";
};

/// What a transition (a happening) does: apply an instantaneous action, start a durative action, end an open one, or
/// apply the next timed initial literals.
enum class TransitionKind : std::uint8_t
{
    APPLY,
    START,
    END,
    TIMED_LITERAL,
};

constexpr std::string_view to_string(TransitionKind kind)
{
    switch (kind)
    {
        case TransitionKind::APPLY:
            return "apply";
        case TransitionKind::START:
            return "start";
        case TransitionKind::END:
            return "end";
        case TransitionKind::TIMED_LITERAL:
            return "timed literal";
    }
    throw std::logic_error("to_string(TransitionKind): unknown kind");
}

/// The tags of a successor and its parts, records read through the state repository of their task (successor_data.hpp):
/// a transition's label, an outcome (a successor state, its metric, its probability and whether it fails), a
/// distribution over outcomes, and a successor (a label and the distributions an adversary picks from).
struct Transition
{
    static constexpr std::string_view name = "Transition";
};

template<TaskKind Kind>
struct OutcomeNode
{
    static constexpr std::string_view name = "OutcomeNode";
};

template<TaskKind Kind>
struct Distribution
{
    static constexpr std::string_view name = "Distribution";
};

template<TaskKind Kind>
struct Successor
{
    static constexpr std::string_view name = "Successor";
};

template<TaskKind Kind>
class StateContext;

struct StateDelta;

/// A successor's parent and its changes from it, as a hint to a state repository's insert (successor_hint.hpp).
template<TaskKind Kind>
struct SuccessorHint;

template<TaskKind Kind>
class SuccessorGenerator;
template<TaskKind Kind>
using SuccessorGeneratorPtr = std::shared_ptr<SuccessorGenerator<Kind>>;

template<TaskKind Kind>
class StateRepository;
template<TaskKind Kind>
using StateRepositoryPtr = std::shared_ptr<StateRepository<Kind>>;

/// A state read unpacked: the view of its builder (a registered state's, which holds its index, or one being built),
/// borrowed, through a state repository of its task. What every reader of a state takes: a node's state reads as it, and
/// a view that shares a builder (SharedStateBuilderView) lends it (urd::borrowed_view).
template<TaskKind Kind>
using StateBuilderView = urd::borrowed_view<State<Kind>, StateRepository<Kind>, urd::builder<State<Kind>, StateRepository<Kind>>>;
/// A state on its own, unpacked: the view of its builder, shared (the builder goes back to its repository's pool with
/// the last copy), through the repository that holds it. What a repository hands out a state as where no node holds it.
template<TaskKind Kind>
using SharedStateBuilderView = urd::shared_view<State<Kind>, StateRepository<Kind>, urd::builder<State<Kind>, StateRepository<Kind>>>;
/// A state's storages being built, borrowed, read through their state repository (which resolves the task's facts
/// through the formalism repository it nests): its fluent facts, derived atoms and numeric values.
template<TaskKind Kind>
using FactStorageBuilderView = urd::borrowed_view<FactStorage<Kind>, StateRepository<Kind>, urd::builder<FactStorage<Kind>, StateRepository<Kind>>>;
template<TaskKind Kind>
using AtomStorageBuilderView = urd::borrowed_view<AtomStorage<Kind>, StateRepository<Kind>, urd::builder<AtomStorage<Kind>, StateRepository<Kind>>>;
template<TaskKind Kind>
using NumericStorageBuilderView = urd::borrowed_view<NumericStorage<Kind>, StateRepository<Kind>, urd::builder<NumericStorage<Kind>, StateRepository<Kind>>>;
template<TaskKind Kind>
using TemporalStorageBuilderView = urd::borrowed_view<TemporalStorage<Kind>, StateRepository<Kind>, urd::builder<TemporalStorage<Kind>, StateRepository<Kind>>>;
/// A successor being built (a generator writes it: its outcome states built, unregistered) and a successor packed (its
/// record: its outcome states' keys), each borrowed or owned, read through the state repository of its task.
template<TaskKind Kind>
using SuccessorBuilder = urd::builder<Successor<Kind>, StateRepository<Kind>>;
template<TaskKind Kind>
using SuccessorBuilderView = urd::borrowed_view<Successor<Kind>, StateRepository<Kind>, SuccessorBuilder<Kind>>;
template<TaskKind Kind>
using OwnedSuccessorBuilderView = urd::owning_view<Successor<Kind>, StateRepository<Kind>, SuccessorBuilder<Kind>>;
template<TaskKind Kind>
using SuccessorRecordView = urd::borrowed_view<Successor<Kind>, StateRepository<Kind>, urd::data<Successor<Kind>>>;
template<TaskKind Kind>
using OwnedSuccessorRecordView = urd::owning_view<Successor<Kind>, StateRepository<Kind>, urd::data<Successor<Kind>>>;
/// A registered state, packed: the view of its record's index, through its repository, which must outlive it (unpack()
/// gives it unpacked, a SharedStateBuilderView).
template<TaskKind Kind>
using StateView = urd::index_view<State<Kind>, StateRepository<Kind>>;
/// A state held as a record of its own (not interned), borrowed: one of the forms a node's state takes (urd::child).
template<TaskKind Kind>
using StateRecordView = urd::borrowed_view<State<Kind>, StateRepository<Kind>, urd::data<State<Kind>>>;
/// A node being built or held (its builder: its state built), and a node packed (its record: its state's key), each
/// borrowed and read through its state repository; likewise a labeled node.
template<TaskKind Kind>
using NodeBuilder = urd::builder<Node<Kind>, StateRepository<Kind>>;
template<TaskKind Kind>
using NodeBuilderView = urd::borrowed_view<Node<Kind>, StateRepository<Kind>, NodeBuilder<Kind>>;
template<TaskKind Kind>
using NodeRecordView = urd::borrowed_view<Node<Kind>, StateRepository<Kind>, urd::data<Node<Kind>>>;
/// A node packed, owned (its record by value) and read through its repository, which must outlive it: what is kept of a
/// node apart from a search (a start node given, a goal node found).
template<TaskKind Kind>
using OwnedNodeRecordView = urd::owning_view<Node<Kind>, StateRepository<Kind>, urd::data<Node<Kind>>>;
/// A node owned whole (its builder by value: its state by its handle) and read through its repository, which must outlive
/// it: a node handed out on its own (as in Python, or a test's list).
template<TaskKind Kind>
using OwnedNodeBuilderView = urd::owning_view<Node<Kind>, StateRepository<Kind>, NodeBuilder<Kind>>;
template<TaskKind Kind>
using LabeledNodeBuilder = urd::builder<LabeledNode<Kind>, StateRepository<Kind>>;
template<TaskKind Kind>
using LabeledNodeBuilderView = urd::borrowed_view<LabeledNode<Kind>, StateRepository<Kind>, LabeledNodeBuilder<Kind>>;
template<TaskKind Kind>
using LabeledNodeRecordView = urd::borrowed_view<LabeledNode<Kind>, StateRepository<Kind>, urd::data<LabeledNode<Kind>>>;
template<TaskKind Kind>
using OwnedLabeledNodeBuilderView = urd::owning_view<LabeledNode<Kind>, StateRepository<Kind>, LabeledNodeBuilder<Kind>>;
template<TaskKind Kind>
using OwnedLabeledNodeRecordView = urd::owning_view<LabeledNode<Kind>, StateRepository<Kind>, urd::data<LabeledNode<Kind>>>;
/// A plan being built or held (its builder), borrowed or owned, and a plan packed (its record), borrowed or owned; each read
/// through its state repository, which must outlive it (its states' builders go back to its pool).
template<TaskKind Kind>
using PlanBuilder = urd::builder<Plan<Kind>, StateRepository<Kind>>;
template<TaskKind Kind>
using PlanBuilderView = urd::borrowed_view<Plan<Kind>, StateRepository<Kind>, PlanBuilder<Kind>>;
template<TaskKind Kind>
using OwnedPlanBuilderView = urd::owning_view<Plan<Kind>, StateRepository<Kind>, PlanBuilder<Kind>>;
template<TaskKind Kind>
using PlanRecordView = urd::borrowed_view<Plan<Kind>, StateRepository<Kind>, urd::data<Plan<Kind>>>;
template<TaskKind Kind>
using OwnedPlanRecordView = urd::owning_view<Plan<Kind>, StateRepository<Kind>, urd::data<Plan<Kind>>>;

template<TaskKind Kind>
class AxiomEvaluator;
template<TaskKind Kind>
using AxiomEvaluatorPtr = std::shared_ptr<AxiomEvaluator<Kind>>;

/// A state builder held by its handle, from its repository's builder storage (get_state_builder), to which it goes back
/// with the handle: what a successor is written into, and what a node holds its state as.
template<TaskKind Kind>
using StateBuilderHandle = urd::handle<urd::builder<State<Kind>, StateRepository<Kind>>>;

/// The free functions over a state repository (state_repository.hpp), declared where its views read them.
template<TaskKind Kind>
StateBuilderHandle<Kind> get_state_builder(const StateRepository<Kind>& repository);
template<TaskKind Kind>
StateBuilderHandle<Kind> unpack(const StateRepository<Kind>& repository, const urd::data<State<Kind>>& packed);
template<TaskKind Kind>
StateBuilderHandle<Kind> get_registered_state(const StateRepository<Kind>& repository, urd::index<State<Kind>> state_index);
template<TaskKind Kind>
SharedStateBuilderView<Kind> share(const StateRepository<Kind>& repository, StateBuilderHandle<Kind> state);

template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class Heuristic;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
using HeuristicPtr = std::shared_ptr<Heuristic<Kind, V>>;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class BlindHeuristic;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
using BlindHeuristicPtr = std::shared_ptr<BlindHeuristic<Kind, V>>;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class GoalCountHeuristic;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
using GoalCountHeuristicPtr = std::shared_ptr<GoalCountHeuristic<Kind, V>>;
template<TaskKind Kind>
class RPGProgram;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class AddRPGHeuristic;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
using AddRPGHeuristicPtr = std::shared_ptr<AddRPGHeuristic<Kind, V>>;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class FFRPGHeuristic;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
using FFRPGHeuristicPtr = std::shared_ptr<FFRPGHeuristic<Kind, V>>;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class MaxRPGHeuristic;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
using MaxRPGHeuristicPtr = std::shared_ptr<MaxRPGHeuristic<Kind, V>>;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class LMCutHeuristic;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
using LMCutHeuristicPtr = std::shared_ptr<LMCutHeuristic<Kind, V>>;

template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class PruningStrategy;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
using PruningStrategyPtr = std::shared_ptr<PruningStrategy<Kind, V>>;

template<TaskKind Kind, typename V = StateBuilderView<Kind>>
class GoalStrategy;
template<TaskKind Kind, typename V = StateBuilderView<Kind>>
using GoalStrategyPtr = std::shared_ptr<GoalStrategy<Kind, V>>;


class Statistics;
class ProgressStatistics;

namespace astar_eager
{
template<TaskKind Kind>
class WorkerEventHandler;
template<TaskKind Kind>
using WorkerEventHandlerPtr = std::unique_ptr<WorkerEventHandler<Kind>>;
template<TaskKind Kind>
class EventHandler;
template<TaskKind Kind>
using EventHandlerPtr = std::shared_ptr<EventHandler<Kind>>;
template<TaskKind Kind>
class DefaultEventHandler;
template<TaskKind Kind>
using DefaultEventHandlerPtr = std::shared_ptr<DefaultEventHandler<Kind>>;
}

namespace gbfs_lazy
{
template<TaskKind Kind>
class WorkerEventHandler;
template<TaskKind Kind>
using WorkerEventHandlerPtr = std::unique_ptr<WorkerEventHandler<Kind>>;
template<TaskKind Kind>
class EventHandler;
template<TaskKind Kind>
using EventHandlerPtr = std::shared_ptr<EventHandler<Kind>>;
template<TaskKind Kind>
class DefaultEventHandler;
template<TaskKind Kind>
using DefaultEventHandlerPtr = std::shared_ptr<DefaultEventHandler<Kind>>;
}

namespace brfs
{
template<TaskKind Kind>
class WorkerEventHandler;
template<TaskKind Kind>
using WorkerEventHandlerPtr = std::unique_ptr<WorkerEventHandler<Kind>>;
template<TaskKind Kind>
class EventHandler;
template<TaskKind Kind>
using EventHandlerPtr = std::shared_ptr<EventHandler<Kind>>;
template<TaskKind Kind>
class DefaultEventHandler;
template<TaskKind Kind>
using DefaultEventHandlerPtr = std::shared_ptr<DefaultEventHandler<Kind>>;
}

namespace iw
{
template<TaskKind Kind>
class EventHandler;
template<TaskKind Kind>
using EventHandlerPtr = std::shared_ptr<EventHandler<Kind>>;
template<TaskKind Kind>
class DefaultEventHandler;
template<TaskKind Kind>
using DefaultEventHandlerPtr = std::shared_ptr<DefaultEventHandler<Kind>>;
}

}

#endif
