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

#ifndef TYR_PLANNING_APPLICABILITY_LIFTED_DECL_HPP_
#define TYR_PLANNING_APPLICABILITY_LIFTED_DECL_HPP_

#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/grounder_decl.hpp"
#include "tyr/planning/applicability_decl.hpp"
#include "tyr/planning/declarations.hpp"

namespace tyr::planning
{
class ApplicabilityContext
{
public:
    ApplicabilityContext(const StateContext<LiftedTag>& state, formalism::planning::GrounderContext& grounder, const formalism::planning::FDRContext& fdr) :
        m_state(&state),
        m_grounder(&grounder),
        m_fdr(&fdr)
    {
    }

    [[nodiscard]] const StateContext<LiftedTag>& state() const noexcept { return *m_state; }
    [[nodiscard]] formalism::planning::GrounderContext& grounder() const noexcept { return *m_grounder; }
    [[nodiscard]] const formalism::planning::FDRContext& fdr() const noexcept { return *m_fdr; }

private:
    const StateContext<LiftedTag>* m_state;
    formalism::planning::GrounderContext* m_grounder;
    const formalism::planning::FDRContext* m_fdr;
};
}

#endif