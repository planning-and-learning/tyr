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

#ifndef TYR_FORMALISM_PLANNING_MUTABLE_CONJUNCTIVE_CONDITION_MATCHER_HPP_
#define TYR_FORMALISM_PLANNING_MUTABLE_CONJUNCTIVE_CONDITION_MATCHER_HPP_

#include "tyr/formalism/planning/mutable/conjunctive_condition.hpp"
#include "tyr/formalism/planning/mutable/literal.hpp"
#include "tyr/formalism/unification/structure_traits.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"

#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/elements/less.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>

namespace tyr::formalism::planning
{

template<FactKind T>
struct TaggedMutableAtomMatchData
{
    struct PredicateData
    {
        urd::Store<urd::sequence<MutableAtom<T>>> atoms;
    };

    urd::Store<urd::map<urd::index<Predicate<T>>, PredicateData>> data;
};

struct MutableAtomMatchData
{
    TaggedMutableAtomMatchData<StaticTag> static_data;
    TaggedMutableAtomMatchData<FluentTag> fluent_data;
};

class ConjunctiveConditionMatcher
{
public:
    ConjunctiveConditionMatcher(const MutableConjunctiveCondition& condition, const MutableAtomMatchData& data) : m_condition(&condition), m_data(&data) {}

    template<typename Callback>
    void for_each_maximal_match([[maybe_unused]] Callback callback) const
    {
    }

private:
    const MutableConjunctiveCondition* m_condition;
    const MutableAtomMatchData* m_data;
};
}

#endif
