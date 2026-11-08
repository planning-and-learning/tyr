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

#ifndef TYR_FORMALISM_PLANNING_MERGE_PLANNING_HPP_
#define TYR_FORMALISM_PLANNING_MERGE_PLANNING_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/interning.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/write_context.hpp"

#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/store.hpp>

/// Datalog bindings merged back into a planning repository (d2p): read through the source view's datalog repository,
/// written through the destination WriteContext, each predicate taken from the caller's mapping (a fluent datalog
/// predicate may become a derived planning one). Records of the same type in both are copied with formalism::copy.
namespace tyr::formalism::planning
{

template<FactKind T_SRC, FactKind T_DST>
std::pair<PredicateRowView<T_DST>, bool> merge_d2p(datalog::PredicateRowView<T_SRC> element,  //
                                                       const urd::Store<urd::map<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>>& predicate_mapping,
                                                       WriteContext<Builder, Repository>& context)
{
    auto binding = planning::checkout<ygg::Row<Predicate<T_DST>>>(context.builder());

    binding->relation = predicate_mapping.at(element.get_relation()).index();
    for (const auto object : element.get_objects())
        binding->values.push_back(object.index());

    return planning::insert(context.destination(), *binding);
}

template<FactKind T_SRC, FactKind T_DST>
std::pair<AtomView<GroundTag, T_DST>, bool> merge_atom_d2p(datalog::PredicateRowView<T_SRC> element,  //
                                                           const urd::Store<urd::map<datalog::PredicateView<T_SRC>, PredicateView<T_DST>>>& predicate_mapping,
                                                           WriteContext<Builder, Repository>& context)
{
    auto atom = planning::checkout<Atom<GroundTag, T_DST>>(context.builder());

    atom->row = merge_d2p<T_SRC, T_DST>(element, predicate_mapping, context).first.data();

    return planning::insert(context.destination(), *atom);
}

}

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
extern template std::pair<PredicateRowView<StaticTag>, bool>
merge_d2p(datalog::PredicateRowView<StaticTag> element,
          const urd::Store<urd::map<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>>& predicate_mapping,
          WriteContext<Builder, Repository>& context);
extern template std::pair<PredicateRowView<FluentTag>, bool>
merge_d2p(datalog::PredicateRowView<FluentTag> element,
          const urd::Store<urd::map<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>>& predicate_mapping,
          WriteContext<Builder, Repository>& context);
extern template std::pair<PredicateRowView<DerivedTag>, bool>
merge_d2p(datalog::PredicateRowView<FluentTag> element,
          const urd::Store<urd::map<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>>& predicate_mapping,
          WriteContext<Builder, Repository>& context);
extern template std::pair<AtomView<GroundTag, StaticTag>, bool>
merge_atom_d2p<StaticTag, StaticTag>(datalog::PredicateRowView<StaticTag> element,
                                     const urd::Store<urd::map<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>>& predicate_mapping,
                                     WriteContext<Builder, Repository>& context);
extern template std::pair<AtomView<GroundTag, FluentTag>, bool>
merge_atom_d2p<FluentTag, FluentTag>(datalog::PredicateRowView<FluentTag> element,
                                     const urd::Store<urd::map<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>>& predicate_mapping,
                                     WriteContext<Builder, Repository>& context);
extern template std::pair<AtomView<GroundTag, DerivedTag>, bool>
merge_atom_d2p<FluentTag, DerivedTag>(datalog::PredicateRowView<FluentTag> element,
                                      const urd::Store<urd::map<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>>& predicate_mapping,
                                      WriteContext<Builder, Repository>& context);
}

#endif

#endif
