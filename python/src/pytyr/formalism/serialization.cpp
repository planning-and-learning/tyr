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

#include "module.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/repository.hpp"

#include <boost/mp11.hpp>
#include <yggdrasil/formalism/row_view.hpp>
#include <yggdrasil/python/serialization.hpp>

namespace tyr::formalism
{
void bind_serialization()
{
    using IndexViews = ygg::python::index_views<Repository, SymbolRepositoryTypes>;
    using RowViews = ygg::python::row_views<Repository, RelationRepositoryTypes>;
    // Terms and function expressions are not among them: std::variants (of views and values), serialized where their
    // holders hold them.
    using Others = boost::mp11::mp_list<FDRFactView<FluentTag>,
                                        PlanningDomain,
                                        PlanningTask<LiftedTag>,
                                        PlanningTask<GroundTag>,
                                        BooleanOperatorKind,
                                        ArithmeticOperatorKind,
                                        NumericEffectOperatorKind,
                                        OptimizationDirection,
                                        RequirementKind>;
    ygg::python::bind_serialization(boost::mp11::mp_append<IndexViews, RowViews, Others> {});
}
}  // namespace tyr::formalism
