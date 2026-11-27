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

#ifndef TYR_PYTHON_PLANNING_ALGORITHMS_STRATEGIES_PRUNING_HPP_
#define TYR_PYTHON_PLANNING_ALGORITHMS_STRATEGIES_PRUNING_HPP_

#include "pytyr/planning/module.hpp"
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/trampoline.h>
#include <string>
#include <tyr/planning/algorithms/strategies/pruning.hpp>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/states/state_view.hpp>
#include <urd/python/bindings.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class PyPruningStrategy : public PruningStrategy<Kind>
{
public:
    using Base = PruningStrategy<Kind>;

    NB_TRAMPOLINE(Base);

    [[nodiscard]] PruningStrategyPtr<Kind> make_worker(urd::index<Worker> index) const override { NB_OVERRIDE(make_worker, index); }

    bool should_prune_state(const StateBuilderView<Kind>& state) override { NB_OVERRIDE(should_prune_state, state); }

    bool should_prune_successor_state(const StateBuilderView<Kind>& state, const StateBuilderView<Kind>& succ_state, bool is_new_succ_state) override
    { NB_OVERRIDE(should_prune_successor_state, state, succ_state, is_new_succ_state); }
};

template<TaskKind Kind>
void bind_pruning_strategy(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = PruningStrategy<Kind>;

    nb::class_<T, PyPruningStrategy<Kind>>(m, name.c_str())  //
        .def(nb::init<>())
        .def("make_worker", &T::make_worker, "worker_index"_a)
        .def("should_prune_state", nb::overload_cast<const StateBuilderView<Kind>&>(&T::should_prune_state), "state"_a, "A search calls it with views it owns, valid during the call: keep one by copying it (a registered state: state.pack(); a state: StateBuilder.assign).")
        .def("should_prune_successor_state",
             nb::overload_cast<const StateBuilderView<Kind>&, const StateBuilderView<Kind>&, bool>(&T::should_prune_successor_state),
             "A search calls it with views it owns, valid during the call: keep one by copying it (a registered state: state.pack(); a state: StateBuilder.assign).",
             "state"_a,
             "succ_state"_a,
             "is_new_succ_state"_a);
}

}  // namespace tyr::planning

#endif
