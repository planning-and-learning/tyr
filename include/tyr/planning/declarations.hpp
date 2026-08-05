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

#include <concepts>
#include <memory>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/execution/declarations.hpp>

namespace tyr::planning
{

struct Worker
{
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
template<TaskKind Kind>
using TaskPtr = std::shared_ptr<Task<Kind>>;

template<TaskKind Kind>
class Node;
template<TaskKind Kind>
struct LabeledNode;

template<TaskKind Kind>
class State;

template<TaskKind Kind>
struct StateContext;

template<TaskKind Kind>
class SuccessorGenerator;
template<TaskKind Kind>
using SuccessorGeneratorPtr = std::shared_ptr<SuccessorGenerator<Kind>>;
template<TaskKind Kind>
class SuccessorGeneratorFactory;

template<TaskKind Kind>
class StateRepository;
template<TaskKind Kind>
using StateRepositoryPtr = std::shared_ptr<StateRepository<Kind>>;

template<TaskKind Kind>
using StateView = ygg::View<ygg::Index<State<Kind>>, StateRepositoryPtr<Kind>>;
template<TaskKind Kind>
class StateRepositoryFactory;

template<TaskKind Kind>
class AxiomEvaluator;
template<TaskKind Kind>
using AxiomEvaluatorPtr = std::shared_ptr<AxiomEvaluator<Kind>>;
template<TaskKind Kind>
class AxiomEvaluatorFactory;

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
