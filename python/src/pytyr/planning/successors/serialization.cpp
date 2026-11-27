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
#include "pytyr/planning/module.hpp"

#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/states/state_repository.hpp"
#include "tyr/planning/successors/distribution_view.hpp"
#include "tyr/planning/successors/outcome_node_view.hpp"
#include "tyr/planning/successors/successor_view.hpp"
#include "tyr/planning/successors/transition_view.hpp"

#include <boost/mp11.hpp>
#include <yggdrasil/python/serialization.hpp>

namespace tyr::planning
{

/// The serialization hooks of a task kind's successor views and its parts' builder views (its label and the label's
/// binding, distributions and outcomes), once they are bound.
template<TaskKind Kind>
void bind_successor_serialization()
{
    using Repository = StateRepository<Kind>;
    ygg::python::bind_serialization(boost::mp11::mp_list<SuccessorBuilderView<Kind>,
                                                         SuccessorRecordView<Kind>,
                                                         urd::borrowed_view<ygg::Row<formalism::Action<LiftedTag>>,
                                                                            Repository,
                                                                            urd::builder<ygg::Row<formalism::Action<LiftedTag>>, Repository>>,
                                                         urd::borrowed_view<Transition, Repository, urd::builder<Transition, Repository>>,
                                                         urd::borrowed_view<Distribution<Kind>, Repository, urd::builder<Distribution<Kind>, Repository>>,
                                                         urd::borrowed_view<OutcomeNode<Kind>, Repository, urd::builder<OutcomeNode<Kind>, Repository>>> {});
}

template void bind_successor_serialization<GroundTag>();
template void bind_successor_serialization<LiftedTag>();

}  // namespace tyr::planning
