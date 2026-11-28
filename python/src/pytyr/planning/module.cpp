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

#include "module.hpp"

#include "algorithms/astar_eager/module.hpp"
#include "algorithms/brfs/module.hpp"
#include "algorithms/gbfs_lazy/module.hpp"
#include "algorithms/iw/module.hpp"
#include "algorithms/siw/module.hpp"
#include "pytyr/module.hpp"
#include "pytyr/planning/ground/module.hpp"
#include "pytyr/planning/lifted/module.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/planning/algorithms/astar_eager.hpp"
#include "tyr/planning/algorithms/search_budget.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/nodes/node_view.hpp"
#include "tyr/planning/states/state_delta.hpp"
#include "tyr/planning/states/storages/temporal_parts.hpp"
#include "tyr/planning/successors/transition_data.hpp"

#include <cstdint>
#include <nanobind/stl/chrono.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <optional>
#include <urd/python/bindings.hpp>
#include <urd/python/sequences/sequence.hpp>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::planning
{
namespace
{
/// What a transition does, and its label (the same classes for both kinds of task).
void bind_transition(nb::module_& m)
{
    nb::enum_<TransitionKind>(m, "TransitionKind")
        .value("APPLY", TransitionKind::APPLY)
        .value("START", TransitionKind::START)
        .value("END", TransitionKind::END)
        .value("TIMED_LITERAL", TransitionKind::TIMED_LITERAL);
    // A state's temporal part (a state's temporal storage's parts): its open durative actions (each its action binding's
    // key: Repository.view gives its row), the roles of its frontier time points (internal numbering), how many timed
    // initial literals happened, and its temporal network over the frontier (the origin first).
    nb::enum_<TemporalRoleKind>(m, "TemporalRoleKind")
        .value("WRITER", TemporalRoleKind::WRITER)
        .value("READER", TemporalRoleKind::READER)
        .value("OVER_ALL", TemporalRoleKind::OVER_ALL)
        .value("NUMERIC_WRITER", TemporalRoleKind::NUMERIC_WRITER)
        .value("NUMERIC_READER", TemporalRoleKind::NUMERIC_READER);
    auto open_action = nb::class_<OpenAction>(m, "OpenAction");
    urd::python::bind_fields(open_action);
    auto role = nb::class_<TemporalRole>(m, "TemporalRole");
    urd::python::bind_fields(role);
    nb::class_<TemporalNetwork>(m, "TemporalNetwork")
        .def("size", &TemporalNetwork::size)
        .def("distance", &TemporalNetwork::distance, nb::arg("i"), nb::arg("j"))
        .def("earliest", &TemporalNetwork::earliest, nb::arg("i"), "The earliest time of time point i.")
        .def("latest", &TemporalNetwork::latest, nb::arg("i"), "The latest time of time point i (inf: unbounded).");
    auto parts = nb::class_<TemporalParts>(m, "TemporalParts");
    urd::python::bind_fields(parts);
    urd::python::bind_index<Transition>(m);
    urd::python::bind_data<Transition>(m);
    // What a task asks of an environment (SuccessorGenerator.features()).
    nb::class_<TaskFeatures>(m, "TaskFeatures")
        .def_ro("oneof", &TaskFeatures::oneof)
        .def_ro("probabilistic", &TaskFeatures::probabilistic)
        .def_ro("temporal", &TaskFeatures::temporal);
}
}  // namespace

/**
 * bind_module_definitions
 */

void bind_module_definitions(nb::module_& m)
{
    using namespace nb::literals;
    urd::python::bind_index<Worker>(m);

    // The exact changes of a successor state from its source, which the generator writes and registration stores.
    nb::class_<StateDelta>(m, "StateDelta").def(nb::init<>());

    nb::class_<SearchBudget>(m, "SearchBudget")
        .def(nb::init<std::optional<ygg::uint_t>, std::optional<std::chrono::steady_clock::duration>>(),
             "max_num_states"_a = nb::none(),
             "max_time"_a = nb::none())
        .def_rw("max_num_states", &SearchBudget::max_num_states)
        .def_rw("max_time", &SearchBudget::max_time);

    /**
     * SearchStatus
     */

    nb::enum_<SearchStatus>(m, "SearchStatus")
        .value("IN_PROGRESS", SearchStatus::IN_PROGRESS)
        .value("OUT_OF_TIME", SearchStatus::OUT_OF_TIME)
        .value("OUT_OF_STATES", SearchStatus::OUT_OF_STATES)
        .value("FAILED", SearchStatus::FAILED)
        .value("EXHAUSTED", SearchStatus::EXHAUSTED)
        .value("CYCLE", SearchStatus::CYCLE)
        .value("SOLVED", SearchStatus::SOLVED)
        .value("UNSOLVABLE", SearchStatus::UNSOLVABLE);

    /**
     * CostMode
     */

    nb::enum_<CostMode>(m, "CostMode").value("UNIT", CostMode::UNIT).value("GENERAL", CostMode::GENERAL);

    nb::enum_<DistHashMode>(m, "DistHashMode").value("RANDOM", DistHashMode::RANDOM).value("LMCUT", DistHashMode::LMCUT);

    nb::enum_<astar_eager::ParallelSearchMode>(m, "ParallelSearchMode")
        .value("SYNCHRONOUS", astar_eager::ParallelSearchMode::SYNCHRONOUS)
        .value("ASYNCHRONOUS", astar_eager::ParallelSearchMode::ASYNCHRONOUS);

    bind_transition(m);

    auto ground_module = m.def_submodule("ground");
    bind_ground_module_definitions(ground_module);

    auto lifted_module = m.def_submodule("lifted");
    bind_lifted_module_definitions(lifted_module);

    auto algorithms_module = m.def_submodule("algorithms");
    const auto bind_algorithm = [&](const char* name, auto bind_ground, auto bind_lifted)
    {
        auto algorithm_module = algorithms_module.def_submodule(name);
        auto ground = algorithm_module.def_submodule("ground");
        bind_ground(ground);
        auto lifted = algorithm_module.def_submodule("lifted");
        bind_lifted(lifted);
    };
    bind_algorithm("astar_eager", astar_eager::bind_ground_module_definitions, astar_eager::bind_lifted_module_definitions);
    bind_algorithm("brfs", brfs::bind_ground_module_definitions, brfs::bind_lifted_module_definitions);
    bind_algorithm("gbfs_lazy", gbfs_lazy::bind_ground_module_definitions, gbfs_lazy::bind_lifted_module_definitions);
    bind_algorithm("iw", iw::bind_ground_module_definitions, iw::bind_lifted_module_definitions);
    bind_algorithm("siw", siw::bind_ground_module_definitions, siw::bind_lifted_module_definitions);

    /**
     * Statistics
     */

    auto statistics_cls = nb::class_<Statistics>(m, "Statistics")
                              .def(nb::init<>())
                              .def("clear", &Statistics::clear)
                              .def("increment_num_generated_successors", &Statistics::increment_num_generated_successors)
                              .def("increment_num_expanded", &Statistics::increment_num_expanded)
                              .def("increment_num_deadends", &Statistics::increment_num_deadends)
                              .def("increment_num_pruned", &Statistics::increment_num_pruned)
                              .def("increment_num_generated_candidates", &Statistics::increment_num_generated_candidates, "transferred"_a)
                              .def("get_num_generated_successors", &Statistics::get_num_generated_successors)
                              .def("get_num_expanded", &Statistics::get_num_expanded)
                              .def("get_num_deadends", &Statistics::get_num_deadends)
                              .def("get_num_pruned", &Statistics::get_num_pruned)
                              .def("get_num_generated_candidates", &Statistics::get_num_generated_candidates)
                              .def("get_num_transferred_candidates", &Statistics::get_num_transferred_candidates)
                              .def("get_communication_overhead", &Statistics::get_communication_overhead)
                              .def("get_num_registered_states", &Statistics::get_num_registered_states)
                              .def("get_state_storage_memory_usage", &Statistics::get_state_storage_memory_usage)
                              .def("get_action_bindings_memory_usage", &Statistics::get_action_bindings_memory_usage)
                              .def("get_predicate_bindings_memory_usage", &Statistics::get_predicate_bindings_memory_usage)
                              .def("get_axiom_bindings_memory_usage", &Statistics::get_axiom_bindings_memory_usage)
                              .def("get_function_bindings_memory_usage", &Statistics::get_function_bindings_memory_usage)
                              .def("get_search_time", &Statistics::get_search_time)
                              .def("get_current_search_time", &Statistics::get_current_search_time)
                              .def("get_idle_time", &Statistics::get_idle_time)
                              .def("get_num_destination_lock_acquisitions", &Statistics::get_num_destination_lock_acquisitions)
                              .def("get_destination_lock_wait_time", &Statistics::get_destination_lock_wait_time)
                              .def("get_destination_lock_hold_time", &Statistics::get_destination_lock_hold_time);
    ygg::add_print(statistics_cls);

    auto progress_statistics_cls = nb::class_<ProgressStatistics>(m, "ProgressStatistics")
                                       .def(nb::init<>())
                                       .def("add_snapshot", &ProgressStatistics::add_snapshot, "statistics"_a)
                                       .def("clear", &ProgressStatistics::clear)
                                       .def("empty", &ProgressStatistics::empty)
                                       .def("size", &ProgressStatistics::size)
                                       .def("get_snapshots", &ProgressStatistics::get_snapshots, nb::rv_policy::copy);
    ygg::add_print(progress_statistics_cls);
}

}  // namespace tyr::planning
