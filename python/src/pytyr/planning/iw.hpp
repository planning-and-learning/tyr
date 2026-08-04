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

#ifndef TYR_PYTHON_PLANNING_IW_HPP_
#define TYR_PYTHON_PLANNING_IW_HPP_

#include "planning.hpp"

namespace tyr::planning::iw
{

template<TaskKind Kind>
class PyEventHandler : public EventHandler<Kind>
{
public:
    using Base = EventHandler<Kind>;

    NB_TRAMPOLINE(Base, 6);

    void on_start_search(ygg::uint_t max_arity) override { NB_OVERRIDE_PURE(on_start_search, max_arity); }

    void on_start_arity(ygg::uint_t arity) override { NB_OVERRIDE_PURE(on_start_arity, arity); }

    void on_end_arity(ygg::uint_t arity, tyr::planning::SearchStatus status) override { NB_OVERRIDE_PURE(on_end_arity, arity, status); }

    void on_end_search(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) override
    {
        NB_OVERRIDE_PURE(on_end_search, status, statistics);
    }

    void on_solved(ygg::uint_t arity) override { NB_OVERRIDE_PURE(on_solved, arity); }

    const Statistics<Kind>& get_statistics() const override { NB_OVERRIDE_PURE(get_statistics); }
};

template<TaskKind Kind>
void bind_statistics(nb::module_& m, const std::string& name)
{
    using T = Statistics<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())  //
                   .def(nb::init<>())
                   .def("clear", &T::clear)
                   .def("set_solution_arity", &T::set_solution_arity, "arity"_a)
                   .def("get_solution_arity", &T::get_solution_arity);
    ygg::add_print(cls);
}

template<TaskKind Kind>
void bind_options(nb::module_& m, const std::string& name)
{
    using T = Options<Kind>;

    nb::class_<T>(m, name.c_str())
        .def(nb::init<>())
        .def_rw("start_node", &T::start_node)
        .def_rw("event_handler", &T::event_handler)
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
        .def_rw("brfs_solver", &T::brfs_solver)
        .def_rw("max_arity", &T::max_arity)
        .def_rw("options", &T::options)
        .def("solve", &T::solve, nb::call_guard<nb::gil_scoped_release>());
}

template<TaskKind Kind>
void bind_find_solution(nb::module_& m, const std::string& py_name)
{
    m.def(
        py_name.c_str(),
        [](brfs::Solver<Kind>& brfs_solver, ygg::uint_t max_arity, const Options<Kind>& options) { return find_solution(brfs_solver, max_arity, options); },
        nb::call_guard<nb::gil_scoped_release>(),
        "brfs_solver"_a,
        "max_arity"_a,
        "options"_a);
}

template<TaskKind Kind>
void bind_event_handler(nb::module_& m, const std::string& name)
{
    using T = EventHandler<Kind>;

    nb::class_<T, PyEventHandler<Kind>>(m, name.c_str())
        .def(nb::init<>())
        .def("on_start_search", &T::on_start_search, "max_arity"_a)
        .def("on_start_arity", &T::on_start_arity, "arity"_a)
        .def("on_end_arity", &T::on_end_arity, "arity"_a, "status"_a)
        .def("on_end_search", &T::on_end_search, "status"_a, "statistics"_a)
        .def("on_solved", &T::on_solved, "arity"_a)
        .def("get_statistics", &T::get_statistics, nb::rv_policy::reference_internal);
}

template<TaskKind Kind>
void bind_default_event_handler(nb::module_& m, const std::string& name)
{
    using T = DefaultEventHandler<Kind>;

    nb::class_<T, EventHandler<Kind>>(m, name.c_str())  //
        .def(nb::init<size_t>(), "verbosity"_a = 0)
        .def("get_statistics", &T::get_statistics, nb::rv_policy::reference_internal);
}

template<TaskKind Kind>
void bind_module_definitions_impl(nb::module_& m)
{
    bind_statistics<Kind>(m, "Statistics");
    bind_options<Kind>(m, "Options");
    bind_solver<Kind>(m, "Solver");
    bind_find_solution<Kind>(m, "find_solution");
    bind_event_handler<Kind>(m, "EventHandler");
    bind_default_event_handler<Kind>(m, "DefaultEventHandler");
}

}  // namespace tyr::planning::iw

#endif
