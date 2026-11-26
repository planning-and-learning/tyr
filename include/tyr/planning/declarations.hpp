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
#include <string_view>
#include <urd/elements/builder.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/handle.hpp>
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

template<TaskKind Kind>
class PackedNode;
template<TaskKind Kind>
struct PackedLabeledNode;

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

/// The tags of the set of distributions a transition draws from while it is built (pick_data.hpp,
/// joint_outcome_data.hpp, joint_distribution_data.hpp, joint_distributions_data.hpp): an outcome nature may pick, a joint
/// outcome (the outcomes picked on its way), a distribution over joint outcomes, and a set of such distributions.
template<TaskKind Kind>
struct Pick
{
    static constexpr std::string_view name = "Pick";
};

template<TaskKind Kind>
struct JointOutcome
{
    static constexpr std::string_view name = "JointOutcome";
};

template<TaskKind Kind>
struct JointDistribution
{
    static constexpr std::string_view name = "JointDistribution";
};

template<TaskKind Kind>
struct JointDistributions
{
    static constexpr std::string_view name = "JointDistributions";
};

template<TaskKind Kind>
class StateContext;

struct StateDelta;

template<TaskKind Kind>
class SuccessorGenerator;
template<TaskKind Kind>
using SuccessorGeneratorPtr = std::shared_ptr<SuccessorGenerator<Kind>>;

template<TaskKind Kind>
class StateRepository;
template<TaskKind Kind>
using StateRepositoryPtr = std::shared_ptr<StateRepository<Kind>>;

/// A registered state, unpacked: the view of its builder, shared from its repository's pool (whose slot keeps the
/// repository alive), through the repository that registered it; it holds the state's index.
template<TaskKind Kind>
using StateView = urd::shared_view<State<Kind>, StateRepository<Kind>, urd::builder<State<Kind>, StateRepository<Kind>>>;
/// A state builder, borrowed, read through a state repository of its task.
template<TaskKind Kind>
using StateBuilderView = urd::borrowed_view<State<Kind>, StateRepository<Kind>, urd::builder<State<Kind>, StateRepository<Kind>>>;
/// A state's storages being built, borrowed, read through their state repository (which resolves the task's facts
/// through the formalism repository it nests): its fluent facts, derived atoms and numeric values.
template<TaskKind Kind>
using FactStorageView = urd::borrowed_view<FactStorage<Kind>, StateRepository<Kind>, urd::builder<FactStorage<Kind>, StateRepository<Kind>>>;
template<TaskKind Kind>
using AtomStorageView = urd::borrowed_view<AtomStorage<Kind>, StateRepository<Kind>, urd::builder<AtomStorage<Kind>, StateRepository<Kind>>>;
template<TaskKind Kind>
using NumericStorageView = urd::borrowed_view<NumericStorage<Kind>, StateRepository<Kind>, urd::builder<NumericStorage<Kind>, StateRepository<Kind>>>;
template<TaskKind Kind>
using TemporalStorageView = urd::borrowed_view<TemporalStorage<Kind>, StateRepository<Kind>, urd::builder<TemporalStorage<Kind>, StateRepository<Kind>>>;
/// A successor being built or handed out (a pooled builder of the successor generator), borrowed, read through the state
/// repository of its task; its outcomes' states are registered (their keys) or built (pooled state builders).
template<TaskKind Kind>
using SuccessorView = urd::borrowed_view<Successor<Kind>, StateRepository<Kind>, urd::builder<Successor<Kind>, StateRepository<Kind>>>;
/// A successor handed out by a successor generator: its pooled builder, held by its handle, which gives it back to the
/// generator's pools when it goes.
template<TaskKind Kind>
using SuccessorHandle = urd::handle<urd::builder<Successor<Kind>, StateRepository<Kind>>>;
/// A registered state, packed: the view of its record's index, through its repository, which must outlive it (a
/// PackedNode keeps it alive; unpack() gives its StateView).
template<TaskKind Kind>
using PackedStateView = urd::index_view<State<Kind>, StateRepository<Kind>>;

template<TaskKind Kind>
class AxiomEvaluator;
template<TaskKind Kind>
using AxiomEvaluatorPtr = std::shared_ptr<AxiomEvaluator<Kind>>;

template<TaskKind Kind>
class Heuristic;
template<TaskKind Kind>
using HeuristicPtr = std::shared_ptr<Heuristic<Kind>>;
template<TaskKind Kind>
class BlindHeuristic;
template<TaskKind Kind>
using BlindHeuristicPtr = std::shared_ptr<BlindHeuristic<Kind>>;
template<TaskKind Kind>
class GoalCountHeuristic;
template<TaskKind Kind>
using GoalCountHeuristicPtr = std::shared_ptr<GoalCountHeuristic<Kind>>;
template<TaskKind Kind>
class RPGProgram;
template<TaskKind Kind>
class AddRPGHeuristic;
template<TaskKind Kind>
using AddRPGHeuristicPtr = std::shared_ptr<AddRPGHeuristic<Kind>>;
template<TaskKind Kind>
class FFRPGHeuristic;
template<TaskKind Kind>
using FFRPGHeuristicPtr = std::shared_ptr<FFRPGHeuristic<Kind>>;
template<TaskKind Kind>
class MaxRPGHeuristic;
template<TaskKind Kind>
using MaxRPGHeuristicPtr = std::shared_ptr<MaxRPGHeuristic<Kind>>;
template<TaskKind Kind>
class LMCutHeuristic;
template<TaskKind Kind>
using LMCutHeuristicPtr = std::shared_ptr<LMCutHeuristic<Kind>>;

template<TaskKind Kind>
class PruningStrategy;
template<TaskKind Kind>
using PruningStrategyPtr = std::shared_ptr<PruningStrategy<Kind>>;

template<TaskKind Kind>
class GoalStrategy;
template<TaskKind Kind>
using GoalStrategyPtr = std::shared_ptr<GoalStrategy<Kind>>;

template<TaskKind Kind>
class Plan;
template<TaskKind Kind>
class PackedPlan;

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
