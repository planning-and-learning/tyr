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

#include "tyr/formalism/planning/merge_planning.hpp"

#include "tyr/formalism/detail/repository_instantiations.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
template std::pair<PredicateRowView<StaticTag>, bool>
merge_d2p(datalog::PredicateRowView<StaticTag> element,
          const urd::Store<urd::map<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>>& predicate_mapping,
          WriteContext<Builder, Repository>& context);
template std::pair<PredicateRowView<FluentTag>, bool>
merge_d2p(datalog::PredicateRowView<FluentTag> element,
          const urd::Store<urd::map<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<Builder, Repository>& context);
template std::pair<PredicateRowView<DerivedTag>, bool>
merge_d2p(datalog::PredicateRowView<FluentTag> element,
          const urd::Store<urd::map<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>>& predicate_mapping,
          WriteContext<Builder, Repository>& context);
template std::pair<AtomView<GroundTag, StaticTag>, bool>
merge_atom_d2p<StaticTag, StaticTag>(datalog::PredicateRowView<StaticTag> element,
                                     const urd::Store<urd::map<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>>& predicate_mapping,
                                     WriteContext<Builder, Repository>& context);
template std::pair<AtomView<GroundTag, FluentTag>, bool>
merge_atom_d2p<FluentTag, FluentTag>(datalog::PredicateRowView<FluentTag> element,
                                     const urd::Store<urd::map<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>>& predicate_mapping,
                                     WriteContext<Builder, Repository>& context);
template std::pair<AtomView<GroundTag, DerivedTag>, bool>
merge_atom_d2p<FluentTag, DerivedTag>(datalog::PredicateRowView<FluentTag> element,
                                      const urd::Store<urd::map<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>>& predicate_mapping,
                                      WriteContext<Builder, Repository>& context);
}

#endif
