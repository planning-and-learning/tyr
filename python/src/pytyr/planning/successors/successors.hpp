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
#include <concepts>
#include <nanobind/nanobind.h>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/states/state_repository.hpp>
#include <tyr/planning/successors/successor_view.hpp>
#include <urd/python/bindings.hpp>

namespace tyr::planning
{

/// A transition's label as records (S4): its builder and views (its record is bound once, for both kinds), alike in every
/// environment. The serialization hooks of a task kind's label views (serialization.cpp: its own source, instantiated
/// per kind).
template<TaskKind Kind>
void bind_successor_serialization();

/// The serialization hooks of a task kind's successor views of environment E and its parts' builder views.
template<NatureEnvironment E, TaskKind Kind>
void bind_environment_successor_serialization();

template<TaskKind Kind>
void bind_successors(nb::module_& m)
{
    using Repository = StateRepository<Kind>;
    urd::python::bind_builder<Transition, Repository>(m);
    urd::python::bind_view<urd::index_view<Transition, Repository>>(m);
    urd::python::bind_record_views<Transition, Repository>(m);
    bind_successor_serialization<Kind>();
}

/// A successor of environment E and its parts (its outcome nodes, and in the general environment its distributions), as
/// records (S4), bound into that environment's submodule: their records, builders and views, each borrowed view bound
/// before the owned one, which lends it.
template<NatureEnvironment E, TaskKind Kind>
void bind_environment_successors(nb::module_& m)
{
    using Repository = StateRepository<Kind>;
    urd::python::bind_record<OutcomeNode<E, Kind>, Repository>(m);
    urd::python::bind_record_views<OutcomeNode<E, Kind>, Repository>(m);
    if constexpr (std::same_as<E, GeneralTag>)
    {
        urd::python::bind_record<Distribution<Kind>, Repository>(m);
        urd::python::bind_record_views<Distribution<Kind>, Repository>(m);
    }
    urd::python::bind_record<Successor<E, Kind>, Repository>(m);
    urd::python::bind_record_views<Successor<E, Kind>, Repository>(m);
    urd::python::bind_view<OwnedSuccessorBuilderView<E, Kind>>(m);
    urd::python::bind_view<OwnedSuccessorRecordView<E, Kind>>(m);
    bind_environment_successor_serialization<E, Kind>();
}

}  // namespace tyr::planning

#endif
