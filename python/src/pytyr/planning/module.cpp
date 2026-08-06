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

#include "ground/module.hpp"
#include "lifted/module.hpp"
#include "tyr/planning/algorithms/astar_eager.hpp"
#include "tyr/planning/algorithms/utils.hpp"
#include "tyr/planning/formatter.hpp"
#include "tyr/planning/worker_index.hpp"

#include <cstdint>
#include <nanobind/stl/chrono.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::planning
{

/**
 * bind_module_definitions
 */

void bind_module_definitions(nb::module_& m)
{
    ygg::bind_index<ygg::Index<Worker>>(m, "WorkerIndex");

    /**
     * SearchStatus
     */

    nb::enum_<SearchStatus>(m, "SearchStatus")
        .value("IN_PROGRESS", SearchStatus::IN_PROGRESS)
        .value("OUT_OF_TIME", SearchStatus::OUT_OF_TIME)
        .value("OUT_OF_MEMORY", SearchStatus::OUT_OF_MEMORY)
        .value("OUT_OF_STATES", SearchStatus::OUT_OF_STATES)
        .value("FAILED", SearchStatus::FAILED)
        .value("EXHAUSTED", SearchStatus::EXHAUSTED)
        .value("CYCLE", SearchStatus::CYCLE)
        .value("SOLVED", SearchStatus::SOLVED)
        .value("UNSOLVABLE", SearchStatus::UNSOLVABLE)
        .export_values();

    /**
     * CostMode
     */

    nb::enum_<CostMode>(m, "CostMode").value("UNIT", CostMode::UNIT).value("GENERAL", CostMode::GENERAL);

    nb::enum_<StateRepositoryMode>(m, "StateRepositoryMode")
        .value("HASH_DISTRIBUTED", StateRepositoryMode::HASH_DISTRIBUTED)
        .value("SHARED", StateRepositoryMode::SHARED);

    nb::enum_<astar_eager::ParallelSearchMode>(m, "ParallelSearchMode")
        .value("SYNCHRONOUS", astar_eager::ParallelSearchMode::SYNCHRONOUS)
        .value("ASYNCHRONOUS", astar_eager::ParallelSearchMode::ASYNCHRONOUS);

    auto ground_module = m.def_submodule("ground");
    bind_ground_module_definitions(ground_module);

    auto lifted_module = m.def_submodule("lifted");
    bind_lifted_module_definitions(lifted_module);

    /**
     * Statistics
     */

    auto statistics_cls = nb::class_<Statistics>(m, "Statistics")
                              .def(nb::init<>())
                              .def("clear", &Statistics::clear)
                              .def("increment_num_generated", &Statistics::increment_num_generated)
                              .def("increment_num_expanded", &Statistics::increment_num_expanded)
                              .def("increment_num_deadends", &Statistics::increment_num_deadends)
                              .def("increment_num_pruned", &Statistics::increment_num_pruned)
                              .def("increment_num_routed_successors", &Statistics::increment_num_routed_successors, "remote"_a)
                              .def("get_num_generated", &Statistics::get_num_generated)
                              .def("get_num_expanded", &Statistics::get_num_expanded)
                              .def("get_num_deadends", &Statistics::get_num_deadends)
                              .def("get_num_pruned", &Statistics::get_num_pruned)
                              .def("get_num_routed_successors", &Statistics::get_num_routed_successors)
                              .def("get_num_remote_routed_successors", &Statistics::get_num_remote_routed_successors)
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

    using ProgressSnapshot = ProgressStatistics::Snapshot;

    auto progress_snapshot_cls =
        nb::class_<ProgressSnapshot>(m, "ProgressStatisticsSnapshot")
            .def(nb::init<uint64_t, uint64_t, uint64_t, uint64_t>(), "num_generated"_a, "num_expanded"_a, "num_deadends"_a, "num_pruned"_a)
            .def("get_num_generated", &ProgressSnapshot::get_num_generated)
            .def("get_num_expanded", &ProgressSnapshot::get_num_expanded)
            .def("get_num_deadends", &ProgressSnapshot::get_num_deadends)
            .def("get_num_pruned", &ProgressSnapshot::get_num_pruned);
    ygg::add_print(progress_snapshot_cls);

    auto progress_statistics_cls = nb::class_<ProgressStatistics>(m, "ProgressStatistics")
                                       .def(nb::init<>())
                                       .def("add_snapshot", &ProgressStatistics::add_snapshot, "statistics"_a)
                                       .def("add_snap_shot", &ProgressStatistics::add_snap_shot, "statistics"_a)
                                       .def("clear", &ProgressStatistics::clear)
                                       .def("empty", &ProgressStatistics::empty)
                                       .def("size", &ProgressStatistics::size)
                                       .def("get_snapshots", &ProgressStatistics::get_snapshots, nb::rv_policy::copy);
    ygg::add_print(progress_statistics_cls);
}

}  // namespace tyr::planning
