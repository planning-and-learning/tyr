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
#ifndef TYR_PYTHON_PLANNING_SUCCESSORS_SUCCESSORS_HPP_
#define TYR_PYTHON_PLANNING_SUCCESSORS_SUCCESSORS_HPP_

#include "pytyr/planning/module.hpp"
#include <nanobind/nanobind.h>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/states/state_repository.hpp>
#include <tyr/planning/successors/successor_view.hpp>
#include <urd/python/bindings.hpp>

namespace tyr::planning
{

/// A successor with outcomes and its parts (its label, distributions and outcome nodes), as records (S4): their records,
/// builders and views (a transition's record is bound once, for both kinds), each borrowed view bound before the owned
/// one, which lends it. The serialization hooks of a task kind's successor views (serialization.cpp: its own source,
/// instantiated per kind).
template<TaskKind Kind>
void bind_successor_serialization();

template<TaskKind Kind>
void bind_successors(nb::module_& m)
{
    using Repository = StateRepository<Kind>;
    urd::python::bind_builder<Transition, Repository>(m);
    urd::python::bind_view<urd::index_view<Transition, Repository>>(m);
    urd::python::bind_view<urd::borrowed_view<Transition, Repository, urd::builder<Transition, Repository>>>(m);
    urd::python::bind_view<urd::borrowed_view<Transition, Repository, urd::data<Transition>>>(m);
    urd::python::bind_index<OutcomeNode<Kind>>(m);
    urd::python::bind_data<OutcomeNode<Kind>>(m);
    urd::python::bind_builder<OutcomeNode<Kind>, Repository>(m);
    urd::python::bind_view<urd::index_view<OutcomeNode<Kind>, Repository>>(m);
    urd::python::bind_view<urd::borrowed_view<OutcomeNode<Kind>, Repository, urd::builder<OutcomeNode<Kind>, Repository>>>(m);
    urd::python::bind_view<urd::borrowed_view<OutcomeNode<Kind>, Repository, urd::data<OutcomeNode<Kind>>>>(m);
    urd::python::bind_index<Distribution<Kind>>(m);
    urd::python::bind_data<Distribution<Kind>>(m);
    urd::python::bind_builder<Distribution<Kind>, Repository>(m);
    urd::python::bind_view<urd::index_view<Distribution<Kind>, Repository>>(m);
    urd::python::bind_view<urd::borrowed_view<Distribution<Kind>, Repository, urd::builder<Distribution<Kind>, Repository>>>(m);
    urd::python::bind_view<urd::borrowed_view<Distribution<Kind>, Repository, urd::data<Distribution<Kind>>>>(m);
    urd::python::bind_index<Successor<Kind>>(m);
    urd::python::bind_data<Successor<Kind>>(m);
    urd::python::bind_builder<Successor<Kind>, Repository>(m);
    urd::python::bind_view<urd::index_view<Successor<Kind>, Repository>>(m);
    urd::python::bind_view<SuccessorBuilderView<Kind>>(m);
    urd::python::bind_view<OwnedSuccessorBuilderView<Kind>>(m);
    urd::python::bind_view<SuccessorRecordView<Kind>>(m);
    urd::python::bind_view<OwnedSuccessorRecordView<Kind>>(m);
    bind_successor_serialization<Kind>();
}

}  // namespace tyr::planning

#endif
