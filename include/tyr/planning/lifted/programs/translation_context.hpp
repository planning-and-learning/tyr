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

#ifndef TYR_PLANNING_LIFTED_PROGRAMS_TRANSLATION_CONTEXT_HPP_
#define TYR_PLANNING_LIFTED_PROGRAMS_TRANSLATION_CONTEXT_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/planning/programs/translation_context.hpp"

#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>

namespace tyr::planning
{

template<>
struct D2PTranslationContext<LiftedTag>
{
    using FluentToDerivedPredicateMapping =
        urd::Store<urd::map<formalism::datalog::PredicateView<formalism::FluentTag>, formalism::planning::PredicateView<formalism::DerivedTag>>>;

    urd::Store<urd::map<formalism::datalog::PredicateView<formalism::StaticTag>, formalism::planning::PredicateView<formalism::StaticTag>>>
        static_to_static_predicate;
    urd::Store<urd::map<formalism::datalog::PredicateView<formalism::FluentTag>, formalism::planning::PredicateView<formalism::FluentTag>>>
        fluent_to_fluent_predicate;
    FluentToDerivedPredicateMapping fluent_to_derived_predicate;
};

/// Mappings select which state inputs are translated; omitted symbols are not inserted.
template<>
struct P2DTranslationContext<LiftedTag>
{
    using FluentToFluentPredicateMapping =
        urd::Store<urd::map<formalism::planning::PredicateView<formalism::FluentTag>, formalism::datalog::PredicateView<formalism::FluentTag>>>;
    using DerivedToFluentPredicateMapping =
        urd::Store<urd::map<formalism::planning::PredicateView<formalism::DerivedTag>, formalism::datalog::PredicateView<formalism::FluentTag>>>;

    using FluentToFluentFunctionMapping =
        urd::Store<urd::map<formalism::planning::FunctionView<formalism::FluentTag>, formalism::datalog::FunctionView<formalism::FluentTag>>>;

    urd::Store<urd::map<formalism::planning::PredicateView<formalism::StaticTag>, formalism::datalog::PredicateView<formalism::StaticTag>>>
        static_to_static_predicate;
    FluentToFluentPredicateMapping fluent_to_fluent_predicate;
    DerivedToFluentPredicateMapping derived_to_fluent_predicate;
    FluentToFluentFunctionMapping fluent_to_fluent_function;
};
}

#endif
