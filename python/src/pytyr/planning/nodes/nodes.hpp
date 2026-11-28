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

#ifndef TYR_PYTHON_PLANNING_NODES_NODES_HPP_
#define TYR_PYTHON_PLANNING_NODES_NODES_HPP_

#include "pytyr/planning/module.hpp"
#include <nanobind/nanobind.h>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/nodes/labeled_node_view.hpp>
#include <tyr/planning/nodes/node_view.hpp>
#include <tyr/planning/states/state_repository.hpp>
#include <yggdrasil/formalism/row_builder.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <urd/python/bindings.hpp>

namespace tyr::planning
{

/// A search node and a labeled node, as records (S4): their records, builders, and views (a builder's borrowed and owned
/// view, a record's borrowed and owned view: a node handed out on its own is owned), each borrowed view bound before
/// the owned one, which lends it.
/// The serialization hooks of a task kind's node and labeled node views (serialization.cpp: its own source, instantiated per kind).
template<TaskKind Kind>
void bind_node_serialization();

template<TaskKind Kind>
void bind_nodes(nb::module_& m)
{
    urd::python::bind_data<Node<Kind>>(m);
    urd::python::bind_builder<Node<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_record_views<Node<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_view<OwnedNodeBuilderView<Kind>>(m);
    urd::python::bind_view<OwnedNodeRecordView<Kind>>(m);
    // A label's binding being built (a borrowed binding copied), read through the repository of its labeled node.
    urd::python::bind_view<urd::borrowed_view<ygg::Row<formalism::Action<LiftedTag>>,
                                              StateRepository<Kind>,
                                              urd::builder<ygg::Row<formalism::Action<LiftedTag>>, StateRepository<Kind>>>>(m);
    urd::python::bind_data<LabeledNode<Kind>>(m);
    urd::python::bind_builder<LabeledNode<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_record_views<LabeledNode<Kind>, StateRepository<Kind>>(m);
    urd::python::bind_view<OwnedLabeledNodeBuilderView<Kind>>(m);
    urd::python::bind_view<OwnedLabeledNodeRecordView<Kind>>(m);
    bind_node_serialization<Kind>();
}

}  // namespace tyr::planning

#endif
