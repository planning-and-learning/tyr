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
#include <concepts>
#include <yggdrasil/python/serialization.hpp>

namespace tyr::planning
{

/// The serialization hooks of a task kind's label views (a transition's builder view and its binding's), once they are
/// bound: alike in every environment.
template<TaskKind Kind>
void bind_successor_serialization()
{
    using Repository = StateRepository<Kind>;
    ygg::python::bind_serialization(
        boost::mp11::mp_list<urd::borrowed_view<ygg::Row<formalism::Action<LiftedTag>>, Repository, urd::builder<ygg::Row<formalism::Action<LiftedTag>>, Repository>>,
                             urd::borrowed_view<Transition, Repository, urd::builder<Transition, Repository>>> {});
}

/// The serialization hooks of a task kind's successor views of environment E and its parts' builder views (its outcomes,
/// and in the general environment its distributions), once they are bound.
template<NatureEnvironment E, TaskKind Kind>
void bind_environment_successor_serialization()
{
    using Repository = StateRepository<Kind>;
    using Outcome = urd::borrowed_view<OutcomeNode<E, Kind>, Repository, urd::builder<OutcomeNode<E, Kind>, Repository>>;
    if constexpr (std::same_as<E, GeneralTag>)
    {
        ygg::python::bind_serialization(
            boost::mp11::mp_list<SuccessorBuilderView<E, Kind>,
                                 SuccessorRecordView<E, Kind>,
                                 urd::borrowed_view<Distribution<Kind>, Repository, urd::builder<Distribution<Kind>, Repository>>,
                                 Outcome> {});
    }
    else
    {
        ygg::python::bind_serialization(boost::mp11::mp_list<SuccessorBuilderView<E, Kind>, SuccessorRecordView<E, Kind>, Outcome> {});
    }
}

template void bind_successor_serialization<GroundTag>();
template void bind_successor_serialization<LiftedTag>();
template void bind_environment_successor_serialization<NondeterministicTag, GroundTag>();
template void bind_environment_successor_serialization<NondeterministicTag, LiftedTag>();
template void bind_environment_successor_serialization<ProbabilisticTag, GroundTag>();
template void bind_environment_successor_serialization<ProbabilisticTag, LiftedTag>();
template void bind_environment_successor_serialization<GeneralTag, GroundTag>();
template void bind_environment_successor_serialization<GeneralTag, LiftedTag>();

}  // namespace tyr::planning
