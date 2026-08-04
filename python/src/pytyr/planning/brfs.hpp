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

#ifndef TYR_PYTHON_PLANNING_BRFS_HPP_
#define TYR_PYTHON_PLANNING_BRFS_HPP_

#include "planning.hpp"

namespace tyr::planning::brfs
{

template<TaskKind Kind>
class PyEventHandler : public EventHandler<Kind>
{
public:
    using Base = EventHandler<Kind>;

    NB_TRAMPOLINE(Base, 9);

    void on_expand_node(const Node<Kind>& node) override { NB_OVERRIDE_PURE(on_expand_node, node); }

    void on_expand_goal_node(const Node<Kind>& node) override { NB_OVERRIDE_PURE(on_expand_goal_node, node); }

    void on_generate_node(const Node<Kind>& source_node, const LabeledNode<Kind>& labeled_succ_node) override
    {
        NB_OVERRIDE_PURE(on_generate_node, source_node, labeled_succ_node);
    }

    void on_prune_node(const Node<Kind>& node) override { NB_OVERRIDE_PURE(on_prune_node, node); }

    void on_prune_node(const Node<Kind>& source_node, const LabeledNode<Kind>& labeled_succ_node) override
    {
        NB_OVERRIDE_PURE(on_prune_node, source_node, labeled_succ_node);
    }

    void on_start_search(const Node<Kind>& node) override { NB_OVERRIDE_PURE(on_start_search, node); }

    void on_finish_layer(ygg::uint_t layer, const tyr::planning::Statistics& statistics) override { NB_OVERRIDE_PURE(on_finish_layer, layer, statistics); }

    void on_end_search(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) override
    {
        NB_OVERRIDE_PURE(on_end_search, status, statistics);
    }

    void on_solved(const Plan<Kind>& plan) override { NB_OVERRIDE_PURE(on_solved, plan); }
};

template<TaskKind Kind>
void bind_options(nb::module_& m, const std::string& name)
{
    using T = Options<Kind>;

    nb::class_<T>(m, name.c_str())
        .def(nb::init<>())
        .def_rw("start_node", &T::start_node)
        .def_rw("event_handler", &T::event_handler)
        .def_rw("pruning_strategy", &T::pruning_strategy)
        .def_rw("goal_strategy", &T::goal_strategy)
        .def_rw("max_num_states", &T::max_num_states)
        .def_rw("max_time", &T::max_time)
        .def_rw("random_seed", &T::random_seed)
        .def_rw("shuffle_labeled_succ_nodes", &T::shuffle_labeled_succ_nodes);
}

template<TaskKind Kind>
void bind_solver(nb::module_& m, const std::string& name)
{
    using T = Solver<Kind>;

    nb::class_<T>(m, name.c_str())
        .def(nb::init<>())
        .def_rw("task", &T::task)
        .def_rw("successor_generator", &T::successor_generator)
        .def_rw("options", &T::options)
        .def("solve", &T::solve, nb::call_guard<nb::gil_scoped_release>());
}

template<TaskKind Kind>
void bind_find_solution(nb::module_& m, const std::string& py_name)
{
    m.def(
        py_name.c_str(),
        [](Task<Kind>& task, SuccessorGenerator<Kind>& successor_generator, const Options<Kind>& options)
        { return find_solution(task, successor_generator, options); },
        nb::call_guard<nb::gil_scoped_release>(),
        "task"_a,
        "successor_generator"_a,
        "options"_a);
}

template<TaskKind Kind>
void bind_event_handler(nb::module_& m, const std::string& name)
{
    using T = EventHandler<Kind>;

    nb::class_<T, PyEventHandler<Kind>>(m, name.c_str())
        .def(nb::init<>())
        .def("on_expand_node", &T::on_expand_node, "node"_a)
        .def("on_expand_goal_node", &T::on_expand_goal_node, "node"_a)
        .def("on_generate_node", &T::on_generate_node, "source_node"_a, "labeled_succ_node"_a)
        .def("on_prune_node", nb::overload_cast<const Node<Kind>&>(&T::on_prune_node), "node"_a)
        .def("on_prune_node", nb::overload_cast<const Node<Kind>&, const LabeledNode<Kind>&>(&T::on_prune_node), "source_node"_a, "labeled_succ_node"_a)
        .def("on_start_search", &T::on_start_search, "node"_a)
        .def("on_finish_layer", &T::on_finish_layer, "layer"_a, "statistics"_a)
        .def("on_end_search", &T::on_end_search, "status"_a, "statistics"_a)
        .def("on_solved", &T::on_solved, "plan"_a);
}

template<TaskKind Kind>
void bind_default_event_handler(nb::module_& m, const std::string& name)
{
    using T = DefaultEventHandler<Kind>;

    nb::class_<T, EventHandler<Kind>>(m, name.c_str())  //
        .def(nb::init<size_t>(), "verbosity"_a = 0)
        .def("get_progress_statistics", &T::get_progress_statistics, nb::rv_policy::reference_internal);
}

template<TaskKind Kind>
void bind_module_definitions_impl(nb::module_& m)
{
    bind_options<Kind>(m, "Options");
    bind_solver<Kind>(m, "Solver");
    bind_find_solution<Kind>(m, "find_solution");
    bind_event_handler<Kind>(m, "EventHandler");
    bind_default_event_handler<Kind>(m, "DefaultEventHandler");
}

}  // namespace tyr::planning::brfs

#endif
