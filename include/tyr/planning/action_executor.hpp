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

#ifndef TYR_PLANNING_ACTION_EXECUTOR_HPP_
#define TYR_PLANNING_ACTION_EXECUTOR_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/applicability_lifted_decl.hpp"
#include "tyr/planning/declarations.hpp"

#include <yggdrasil/core/itertools.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning
{

class ActionExecutor
{
public:
    ActionExecutor() = default;

    // Ground action API

    template<TaskKind Kind>
    bool is_applicable(::tyr::formalism::planning::GroundActionView action, const StateContext<Kind>& state);

    template<TaskKind Kind>
    Node<Kind>
    apply_action(const StateContext<Kind>& state_context, ::tyr::formalism::planning::GroundActionView action, StateRepository<Kind>& state_repository);

    // Lifted action API

    bool is_applicable(::tyr::formalism::planning::ActionView action,
                       const StateContext<LiftedTag>& state_context,
                       ::tyr::formalism::planning::GrounderContext& grounder,
                       const ::tyr::formalism::planning::FDRContext& fdr);

    Node<LiftedTag> apply_action(const StateContext<LiftedTag>& state_context,
                                 ::tyr::formalism::planning::ActionView action,
                                 ::tyr::formalism::planning::GrounderContext& grounder,
                                 ::tyr::formalism::planning::FDRContext& fdr,
                                 StateRepository<LiftedTag>& state_repository);

private:
    ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> m_del_effects;
    ygg::DataList<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> m_add_effects;
    ::tyr::formalism::planning::EffectFamilyList m_effect_families;
    ygg::itertools::cartesian_set::Workspace<ygg::Index<::tyr::formalism::Object>> m_cartesian_workspace;
};
}

#endif
