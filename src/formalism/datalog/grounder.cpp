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

#include "tyr/formalism/datalog/grounder.hpp"

#include "tyr/formalism/detail/repository_instantiations.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism
{

template std::pair<PredicateRowView<StaticTag>, bool> ground_binding(AtomView<LiftedTag, StaticTag> element, GroundingContext<Builder, Repository>& context);
template std::pair<PredicateRowView<FluentTag>, bool> ground_binding(AtomView<LiftedTag, FluentTag> element, GroundingContext<Builder, Repository>& context);
template std::pair<PredicateRowView<DerivedTag>, bool> ground_binding(AtomView<LiftedTag, DerivedTag> element, GroundingContext<Builder, Repository>& context);

template std::pair<FunctionRowView<StaticTag>, bool> ground_binding(FunctionTermView<LiftedTag, StaticTag> element,
                                                                    GroundingContext<Builder, Repository>& context);
template std::pair<FunctionRowView<FluentTag>, bool> ground_binding(FunctionTermView<LiftedTag, FluentTag> element,
                                                                    GroundingContext<Builder, Repository>& context);

template std::optional<FunctionRowView<StaticTag>> try_ground_binding(FunctionTermView<LiftedTag, StaticTag> element,
                                                                      GroundingContext<Builder, Repository>& context);
template std::optional<FunctionRowView<FluentTag>> try_ground_binding(FunctionTermView<LiftedTag, FluentTag> element,
                                                                      GroundingContext<Builder, Repository>& context);

template std::optional<PredicateRowView<StaticTag>> try_ground_binding(AtomView<LiftedTag, StaticTag> element, GroundingContext<Builder, Repository>& context);
template std::optional<PredicateRowView<FluentTag>> try_ground_binding(AtomView<LiftedTag, FluentTag> element, GroundingContext<Builder, Repository>& context);
template std::optional<PredicateRowView<DerivedTag>> try_ground_binding(AtomView<LiftedTag, DerivedTag> element,
                                                                        GroundingContext<Builder, Repository>& context);

}

#endif
