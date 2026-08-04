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

#ifndef TYR_PYTHON_PLANNING_SIW_HPP_
#define TYR_PYTHON_PLANNING_SIW_HPP_

#include "planning.hpp"

namespace tyr::planning::siw
{

template<TaskKind Kind>
class PyEventHandler : public EventHandler<Kind>
{
public:
    using Base = EventHandler<Kind>;

    NB_TRAMPOLINE(Base, 7);

    void on_start_search() override { NB_OVERRIDE_PURE(on_start_search); }

    void on_start_subsearch(ygg::uint_t subsearch_index) override { NB_OVERRIDE_PURE(on_start_subsearch, subsearch_index); }

    void add_subsearch_statistics(const tyr::planning::Statistics& search_statistics, const iw::Statistics<Kind>& solver_statistics) override
    {
        NB_OVERRIDE_PURE(add_subsearch_statistics, search_statistics, solver_statistics);
    }

    void on_end_subsearch(ygg::uint_t subsearch_index, tyr::planning::SearchStatus status) override
    {
        NB_OVERRIDE_PURE(on_end_subsearch, subsearch_index, status);
    }

    void on_end_search(tyr::planning::SearchStatus status, const tyr::planning::Statistics& statistics) override
    {
        NB_OVERRIDE_PURE(on_end_search, status, statistics);
    }

    void on_solved(const Plan<Kind>& plan) override { NB_OVERRIDE_PURE(on_solved, plan); }

    const Statistics<Kind>& get_statistics() const override { NB_OVERRIDE_PURE(get_statistics); }
};

template<TaskKind Kind>
void bind_statistics(nb::module_& m, const std::string& name)
{
    using T = Statistics<Kind>;

    auto cls = nb::class_<T>(m, name.c_str())
                   .def(nb::init<>())
                   .def("clear", &T::clear)
                   .def("add_effective_width", &T::add_effective_width, "width"_a)
                   .def("get_maximum_effective_width", &T::get_maximum_effective_width)
                   .def("get_average_effective_width", &T::get_average_effective_width)
                   .def("get_num_solved_subsearches", &T::get_num_solved_subsearches);
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
        .def_rw("subgoal_strategy", &T::subgoal_strategy)
        .def_rw("goal_strategy", &T::goal_strategy)
        .def_rw("max_num_subsearches", &T::max_num_subsearches);
}

template<TaskKind Kind>
void bind_solver(nb::module_& m, const std::string& name)
{
    using T = Solver<Kind>;

    nb::class_<T>(m, name.c_str())
        .def(nb::init<>())
        .def_rw("iw_solver", &T::iw_solver)
        .def_rw("options", &T::options)
        .def("solve", &T::solve, nb::call_guard<nb::gil_scoped_release>());
}

template<TaskKind Kind>
void bind_find_solution(nb::module_& m, const std::string& py_name)
{
    m.def(
        py_name.c_str(),
        [](iw::Solver<Kind>& iw_solver, const Options<Kind>& options) { return find_solution(iw_solver, options); },
        nb::call_guard<nb::gil_scoped_release>(),
        "iw_solver"_a,
        "options"_a);
}

template<TaskKind Kind>
void bind_event_handler(nb::module_& m, const std::string& name)
{
    using T = EventHandler<Kind>;

    nb::class_<T, PyEventHandler<Kind>>(m, name.c_str())
        .def(nb::init<>())
        .def("on_start_search", &T::on_start_search)
        .def("on_start_subsearch", &T::on_start_subsearch, "subsearch_index"_a)
        .def("add_subsearch_statistics", &T::add_subsearch_statistics, "search_statistics"_a, "solver_statistics"_a)
        .def("on_end_subsearch", &T::on_end_subsearch, "subsearch_index"_a, "status"_a)
        .def("on_end_search", &T::on_end_search, "status"_a, "statistics"_a)
        .def("on_solved", &T::on_solved, "plan"_a)
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

}  // namespace tyr::planning::siw

#endif
