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

#ifndef TYR_DATALOG_SOLVER_HPP_
#define TYR_DATALOG_SOLVER_HPP_

#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"

#include <concepts>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::datalog
{

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void compute_model(ProgramExecutionContext<GroundTag, AP, TP, CP>& ctx);

template<AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void compute_model(ProgramExecutionContext<LiftedTag, AP, TP, CP>& ctx);

template<TaskKind Kind, AnnotationPolicyConcept AP, TerminationPolicyConcept TP, RuleCostPolicyConcept CP>
void execute_model(ProgramExecutionContext<Kind, AP, TP, CP>& ctx, ygg::ExecutionContext& execution_context)
{
    if constexpr (std::same_as<Kind, LiftedTag>)
        ctx.set_num_threads(execution_context.get_num_threads());
    execution_context.arena().execute([&] { compute_model(ctx); });
}

}

#endif
