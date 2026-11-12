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

#ifndef TYR_ANALYSIS_LISTENERS_HPP_
#define TYR_ANALYSIS_LISTENERS_HPP_

#include "tyr/analysis/declarations.hpp"
#include "tyr/analysis/stratification.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <urd/elements/equal_to.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/core/family.hpp>

namespace tyr::analysis
{

using RuleIndexSet = urd::Store<urd::set<urd::index<formalism::datalog::Rule<LiftedTag>>>>;

struct TypedListenerStratum
{
    urd::Store<urd::map<urd::index<formalism::Predicate<formalism::FluentTag>>, RuleIndexSet>> predicates;
    urd::Store<urd::map<urd::index<formalism::Function<formalism::FluentTag>>, RuleIndexSet>> functions;
};

using ListenerStratum = TypedListenerStratum;

struct ListenerStrata
{
    urd::Store<urd::sequence<ListenerStratum>> data;
};

ListenerStrata compute_listeners(const RuleStrata& strata, const formalism::datalog::Repository& context);
}

#endif
