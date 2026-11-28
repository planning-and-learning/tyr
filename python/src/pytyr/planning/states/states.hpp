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

#ifndef TYR_PYTHON_PLANNING_STATES_STATES_HPP_
#define TYR_PYTHON_PLANNING_STATES_STATES_HPP_

#include "pytyr/formalism/conversions.hpp"
#include "pytyr/planning/module.hpp"

#include <memory>
#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/vector.h>
#include <string>
#include <tyr/planning/axioms/axiom_evaluator.hpp>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/states/state_builder.hpp>
#include <tyr/planning/states/state_data.hpp>
#include <tyr/planning/states/state_repository.hpp>
#include <tyr/planning/states/storages/policies/policy.hpp>
#include <tyr/planning/tasks/task.hpp>
#include <urd/python/bindings.hpp>
#include <urd/python/sequences/sequence.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::planning
{

/// The state records and their storages (S4): records, builders and views.
template<TaskKind Kind>
void bind_states(nb::module_& m)
{
    urd::python::bind_index<FactStorage<Kind>>(m);
    urd::python::bind_index<AtomStorage<Kind>>(m);
    urd::python::bind_index<NumericStorage<Kind>>(m);
    urd::python::bind_index<TemporalStorage<Kind>>(m);
    urd::python::bind_data<FactStorage<Kind>>(m);
    urd::python::bind_data<AtomStorage<Kind>>(m);
    urd::python::bind_data<NumericStorage<Kind>>(m);
    urd::python::bind_data<TemporalStorage<Kind>>(m);
    urd::python::bind_builder<FactStorage<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_builder<AtomStorage<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_builder<NumericStorage<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_builder<TemporalStorage<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_view<FactStorageBuilderView<Kind>>(m);
    urd::python::bind_view<AtomStorageBuilderView<Kind>>(m);
    urd::python::bind_view<NumericStorageBuilderView<Kind>>(m);
    urd::python::bind_view<TemporalStorageBuilderView<Kind>>(m);
    urd::python::bind_record<State<Kind>, StateRepository<Kind>>(m);
    // Its builder's view, and the view of a state held as a record of its own, not interned: the forms a node's state
    // takes (urd::child).
    urd::python::bind_record_views<State<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_view<SharedStateBuilderView<Kind>>(m);
    bind_state_serialization<Kind>();
}

template<TaskKind Kind>
void bind_state_repository(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = StateRepository<Kind>;

    nb::class_<T>(m, name.c_str())  //
        .def(nb::new_(&T::create), "task"_a, "concurrent"_a = false)
        .def("get_worker_index", &T::get_worker_index)
        .def("layer", &T::layer)
        .def(
            "get_initial_state",
            [](T& self, AxiomEvaluator<Kind>& axiom_evaluator)
            { return ::tyr::planning::share(self, ::tyr::planning::get_initial_state(self, axiom_evaluator)); },
            "axiom_evaluator"_a)
        .def(
            "get_state_builder",
            [](const T& self) { return urd::clone(*::tyr::planning::get_state_builder(self)); },
            "A builder of a state of this repository, emptied: what a successor is written into.")
        .def(
            "get_registered_state",
            [](const T& self, urd::index<State<Kind>> state_index)
            { return ::tyr::planning::share(self, ::tyr::planning::get_registered_state(self, state_index)); },
            "state_index"_a)
        .def("is_concurrent", &T::is_concurrent)
        .def("num_states", [](const T& self) { return ::tyr::planning::num_states(self); })
        .def("memory_usage", &T::memory_usage)
        .def(
            "create_state",
            [](T& self,
               AxiomEvaluator<Kind>& axiom_evaluator,
               const urd::Store<urd::sequence<urd::data<formalism::FDRFact<formalism::FluentTag>>>>& fluent_facts,
               const urd::Store<urd::sequence<std::pair<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>, ygg::float_t>>>& fterm_values)
            { return ::tyr::planning::share(self, ::tyr::planning::create_state(self, axiom_evaluator, fluent_facts, fterm_values)); },
            "axiom_evaluator"_a,
            "fluent_facts"_a,
            "fterm_values"_a)
        .def(
            "create_state",
            [](T& self,
               AxiomEvaluator<Kind>& axiom_evaluator,
               const urd::Store<urd::sequence<formalism::FDRFactView<formalism::FluentTag>>>& fluent_facts,
               const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>& fterm_values)
            { return ::tyr::planning::share(self, ::tyr::planning::create_state(self, axiom_evaluator, fluent_facts, fterm_values)); },
            "axiom_evaluator"_a,
            "fluent_facts"_a,
            "fterm_values"_a)
        .def(
            "create_state",
            [](T& self,
               AxiomEvaluator<Kind>& axiom_evaluator,
               const urd::Store<urd::sequence<urd::owning_view<formalism::FDRFact<formalism::FluentTag>, formalism::Repository>>>& fluent_facts,
               const urd::Store<urd::sequence<formalism::FunctionTermViewValuePair<GroundTag, formalism::FluentTag>>>& fterm_values)
            { return ::tyr::planning::share(self, ::tyr::planning::create_state(self, axiom_evaluator, fluent_facts, fterm_values)); },
            "axiom_evaluator"_a,
            "fluent_facts"_a,
            "fterm_values"_a);
}

}  // namespace tyr::planning

#endif
