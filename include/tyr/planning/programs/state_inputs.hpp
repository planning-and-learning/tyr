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

#ifndef TYR_PLANNING_PROGRAMS_STATE_INPUTS_HPP_
#define TYR_PLANNING_PROGRAMS_STATE_INPUTS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/repository.hpp"

#include <concepts>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>

namespace tyr::planning
{

/// What a program of one action schema reads of a state: the task's fluent and derived predicates and fluent functions
/// its rules read (a whole program reads all of a state). A program is a layer over its task's repository, so these are
/// the task's own records.
struct StateInputs
{
    urd::Store<urd::set<formalism::PredicateView<formalism::FluentTag>>> fluent_predicates;
    urd::Store<urd::set<formalism::PredicateView<formalism::DerivedTag>>> derived_predicates;
    urd::Store<urd::set<formalism::FunctionView<formalism::FluentTag>>> fluent_functions;

    [[nodiscard]] bool reads(formalism::PredicateView<formalism::FluentTag> predicate) const { return fluent_predicates.contains(predicate); }

    [[nodiscard]] bool reads(formalism::PredicateView<formalism::DerivedTag> predicate) const { return derived_predicates.contains(predicate); }

    [[nodiscard]] bool reads(formalism::FunctionView<formalism::FluentTag> function) const { return fluent_functions.contains(function); }
};

/// What a whole program reads of a state: all of it.
struct AllStateInputs
{
    [[nodiscard]] static bool reads(formalism::PredicateView<formalism::FluentTag>) noexcept { return true; }

    [[nodiscard]] static bool reads(formalism::PredicateView<formalism::DerivedTag>) noexcept { return true; }

    [[nodiscard]] static bool reads(formalism::FunctionView<formalism::FluentTag>) noexcept { return true; }
};

/// What a program reads of a state: StateInputs or AllStateInputs.
template<typename R>
concept StateInputsConcept = requires(const R& inputs,
                                      formalism::PredicateView<formalism::FluentTag> fluent_predicate,
                                      formalism::PredicateView<formalism::DerivedTag> derived_predicate,
                                      formalism::FunctionView<formalism::FluentTag> function) {
    { inputs.reads(fluent_predicate) } -> std::same_as<bool>;
    { inputs.reads(derived_predicate) } -> std::same_as<bool>;
    { inputs.reads(function) } -> std::same_as<bool>;
};

/// The action whose applicability atoms each (derived) predicate holds (action and ground task programs).
using AppPredicateToActionMapping = urd::Store<urd::map<formalism::PredicateView<formalism::DerivedTag>, formalism::ActionView<LiftedTag>>>;

/// The predicate of each conditional effect with variables or a condition, whose atoms are the bindings of its action
/// followed by the objects of the effect's variables for which the effect fires.
/// Keyed by the action too, since actions can share an interned conditional effect.
using ConditionalEffectToPredicateMapping = urd::Store<
    urd::map<std::pair<formalism::ActionView<LiftedTag>, formalism::ConditionalEffectView<LiftedTag>>, formalism::PredicateView<formalism::DerivedTag>>>;

}

#endif
