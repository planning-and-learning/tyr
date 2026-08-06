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

#ifndef TYR_EXE_SEARCH_OUTPUT_HPP_
#define TYR_EXE_SEARCH_OUTPUT_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/algorithms/utils.hpp"

#include <cstddef>
#include <fmt/ostream.h>
#include <iostream>
#include <yggdrasil/core/chrono.hpp>

namespace tyr::cli
{

inline void print_summary(const formalism::planning::Repository& repository)
{
    std::cout << "[Total] Number of objects: " << repository.template size<formalism::Object>() << std::endl;
    std::cout << "[Total] Number of fluent atoms: " << repository.template size<formalism::planning::GroundAtom<formalism::FluentTag>>() << std::endl;
    std::cout << "[Total] Number of derived atoms: " << repository.template size<formalism::planning::GroundAtom<formalism::DerivedTag>>() << std::endl;
    std::cout << "[Total] Number of fluent fterms: " << repository.template size<formalism::planning::GroundFunctionTerm<formalism::FluentTag>>() << std::endl;
    std::cout << "[Total] Action bindings memory usage: " << repository.template memory_usage<formalism::RelationBinding<formalism::planning::Action>>()
              << " bytes" << std::endl;
    std::cout << "[Total] Predicate bindings memory usage: "
              << repository.template memory_usage<formalism::RelationBinding<formalism::Predicate<formalism::StaticTag>>>()
                     + repository.template memory_usage<formalism::RelationBinding<formalism::Predicate<formalism::FluentTag>>>()
                     + repository.template memory_usage<formalism::RelationBinding<formalism::Predicate<formalism::DerivedTag>>>()
              << " bytes" << std::endl;
    std::cout << "[Total] Axiom bindings memory usage: " << repository.template memory_usage<formalism::RelationBinding<formalism::planning::Axiom>>()
              << " bytes" << std::endl;
    std::cout << "[Total] Function bindings memory usage: "
              << repository.template memory_usage<formalism::RelationBinding<formalism::Function<formalism::StaticTag>>>()
                     + repository.template memory_usage<formalism::RelationBinding<formalism::Function<formalism::FluentTag>>>()
                     + repository.template memory_usage<formalism::RelationBinding<formalism::Function<formalism::AuxiliaryTag>>>()
              << " bytes" << std::endl;
}

template<TaskKind Kind>
void print_search_statistics(const planning::SearchResult<Kind>& result)
{
    fmt::print(std::cout, "[Search] Worker utilization: {}\n", result.get_worker_utilization());
    for (std::size_t i = 0; i < result.worker_statistics.size(); ++i)
    {
        const auto& worker = result.worker_statistics[i];
        fmt::print(std::cout,
                   "[Search] Worker {}: idle={} ns, expanded={}, accepted={}, deadends={}, pruned={}, registered={}, state_storage={} bytes\n",
                   i,
                   ygg::to_ns(worker.get_idle_time()),
                   worker.get_num_expanded(),
                   worker.get_num_accepted_successors(),
                   worker.get_num_deadends(),
                   worker.get_num_pruned(),
                   worker.get_num_registered_states(),
                   worker.get_state_storage_memory_usage());
        fmt::print(std::cout,
                   "[Search] Worker {} communication: generated={}, transferred={}\n",
                   i,
                   worker.get_num_generated_successors(),
                   worker.get_num_transferred_successors());
        fmt::print(std::cout,
                   "[Search] Worker {} destination lock: acquisitions={}, wait={} ns, hold={} ns\n",
                   i,
                   worker.get_num_destination_lock_acquisitions(),
                   ygg::to_ns(worker.get_destination_lock_wait_time()),
                   ygg::to_ns(worker.get_destination_lock_hold_time()));
    }
}

}

#endif
