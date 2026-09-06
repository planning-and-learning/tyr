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

#include "tyr/analysis/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/planning/applicability_lifted_decl.hpp"
#include "tyr/planning/declarations.hpp"

#include <yggdrasil/core/types.hpp>

namespace tyr::planning
{

class ActionExecutor
{
public:
    ActionExecutor() = default;

    // Ground action API

    template<TaskKind Kind>
    bool is_applicable(formalism::planning::ActionView<GroundTag> action, const StateContext<Kind>& state);

    template<TaskKind Kind>
    bool is_applicable_if_fires(formalism::planning::ActionView<GroundTag> action, const StateContext<Kind>& state);

    /// Applies the action into caller-owned storage without interning the successor; returns the updated auxiliary metric value.
    template<TaskKind Kind>
    ygg::float_t apply_action_unregistered(const StateContext<Kind>& state_context,
                                           formalism::planning::ActionView<GroundTag> action,
                                           ygg::Builder<State<Kind>>& successor_state_builder);

    // Lifted action API

    bool is_applicable(formalism::planning::ActionView<LiftedTag> action,
                       const StateContext<LiftedTag>& state_context,
                       formalism::planning::GrounderContext& grounder,
                       const formalism::planning::FDRContext& fdr);

    bool is_applicable_if_fires(formalism::planning::ActionView<LiftedTag> action,
                                const StateContext<LiftedTag>& state_context,
                                formalism::planning::GrounderContext& grounder,
                                const formalism::planning::FDRContext& fdr);

    /// Applies the action into caller-owned storage without interning the successor; returns the updated auxiliary metric value.
    ygg::float_t apply_action_unregistered(const StateContext<LiftedTag>& state_context,
                                           formalism::planning::ActionView<LiftedTag> action,
                                           formalism::planning::GrounderContext& grounder,
                                           formalism::planning::FDRContext& fdr,
                                           ygg::Builder<State<LiftedTag>>& successor_state_builder);

private:
    ygg::DataList<formalism::planning::FDRFact<formalism::FluentTag>> m_del_effects;
    ygg::DataList<formalism::planning::FDRFact<formalism::FluentTag>> m_add_effects;
    formalism::planning::EffectFamilyList m_effect_families;
    analysis::CompatibilityWorkspace m_compatibility_workspace;
};
}

#endif
