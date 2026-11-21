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

#ifndef TYR_PLANNING_STATE_VIEW_CONCEPTS_HPP_
#define TYR_PLANNING_STATE_VIEW_CONCEPTS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/planning/declarations.hpp"

#include <concepts>
#include <ranges>
#include <type_traits>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <utility>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{
/**
 * IterableStateConcept
 */

template<class R, class Tag>
concept AtomRangeConcept =
    std::ranges::input_range<R> && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, urd::index<formalism::Atom<GroundTag, Tag>>>;

template<class R, class Tag>
concept FactRangeConcept = std::ranges::input_range<R> && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, urd::data<formalism::FDRFact<Tag>>>;

template<class R, class Tag>
concept FunctionTermValueRangeConcept =
    std::ranges::input_range<R>
    && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, std::pair<urd::index<formalism::FunctionTerm<GroundTag, Tag>>, ygg::float_t>>;

template<typename T>
concept IterableStateConcept = requires(const std::remove_reference_t<T>& cs) {
    { cs.template get_atom_indices<formalism::StaticTag>() } -> AtomRangeConcept<formalism::StaticTag>;
    { cs.get_fluent_fact_index_values() } -> FactRangeConcept<formalism::FluentTag>;
    { cs.template get_atom_indices<formalism::FluentTag>() } -> AtomRangeConcept<formalism::FluentTag>;
    { cs.template get_atom_indices<formalism::DerivedTag>() } -> AtomRangeConcept<formalism::DerivedTag>;
    { cs.template get_fterm_index_values<formalism::StaticTag>() } -> FunctionTermValueRangeConcept<formalism::StaticTag>;
    { cs.template get_fterm_index_values<formalism::FluentTag>() } -> FunctionTermValueRangeConcept<formalism::FluentTag>;
};

/**
 * IterableViewStateConcept
 */

template<class R, class Tag>
concept AtomViewRangeConcept =
    std::ranges::input_range<R> && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, formalism::AtomView<GroundTag, Tag>>;

/// Views of facts, however they hold them (a state's facts are computed on the fly: each view owns its fact).
template<class R, class Tag>
concept FactViewRangeConcept =
    std::ranges::input_range<R> && std::same_as<typename std::remove_cvref_t<std::ranges::range_value_t<R>>::value_type, formalism::FDRFact<Tag>>;

template<class R, class Tag>
concept FunctionTermViewValueRangeConcept =
    std::ranges::input_range<R> && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, formalism::FunctionTermViewValuePair<GroundTag, Tag>>;

template<typename T>
concept IterableViewStateConcept = requires(const std::remove_reference_t<T>& cs) {
    { cs.template get_atoms<formalism::StaticTag>() } -> AtomViewRangeConcept<formalism::StaticTag>;
    { cs.get_fluent_facts() } -> FactViewRangeConcept<formalism::FluentTag>;
    { cs.template get_atoms<formalism::FluentTag>() } -> AtomViewRangeConcept<formalism::FluentTag>;
    { cs.template get_atoms<formalism::DerivedTag>() } -> AtomViewRangeConcept<formalism::DerivedTag>;
    { cs.template get_fterm_values<formalism::StaticTag>() } -> FunctionTermViewValueRangeConcept<formalism::StaticTag>;
    { cs.template get_fterm_values<formalism::FluentTag>() } -> FunctionTermViewValueRangeConcept<formalism::FluentTag>;
};

/// State contents and task context, accepting values and references without requiring repository identity; compared
/// and hashed by its identity (urd::Identifiable).
template<typename T, typename Kind>
concept StateViewConcept = TaskKind<Kind> && IterableStateConcept<T> && IterableViewStateConcept<T> && urd::Identifiable<std::remove_cvref_t<T>>
                           && requires(const std::remove_reference_t<T>& state,
                                       urd::index<formalism::FDRVariable<formalism::FluentTag>> variable,
                                       urd::index<formalism::FunctionTerm<GroundTag, formalism::StaticTag>> static_fterm,
                                       urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>> fluent_fterm,
                                       urd::index<formalism::Atom<GroundTag, formalism::StaticTag>> static_atom,
                                       urd::index<formalism::Atom<GroundTag, formalism::DerivedTag>> derived_atom,
                                       formalism::FDRVariableView<formalism::FluentTag> variable_view,
                                       formalism::FunctionTermView<GroundTag, formalism::StaticTag> static_fterm_view,
                                       formalism::FunctionTermView<GroundTag, formalism::FluentTag> fluent_fterm_view,
                                       formalism::AtomView<GroundTag, formalism::StaticTag> static_atom_view,
                                       formalism::AtomView<GroundTag, formalism::FluentTag> fluent_atom_view,
                                       formalism::AtomView<GroundTag, formalism::DerivedTag> derived_atom_view,
                                       formalism::PredicateView<formalism::StaticTag> static_predicate,
                                       formalism::PredicateView<formalism::FluentTag> fluent_predicate,
                                       formalism::PredicateView<formalism::DerivedTag> derived_predicate) {
                                  { state.context() } -> std::same_as<const StateRepository<Kind>&>;
                                  { state.get_formalism_repository() } -> std::same_as<const formalism::RepositoryPtr&>;
                                  { state.value(variable) } -> std::same_as<formalism::FDRValue>;
                                  { state.value(static_fterm) } -> std::same_as<ygg::float_t>;
                                  { state.value(fluent_fterm) } -> std::same_as<ygg::float_t>;
                                  { state.test(static_atom) } -> std::same_as<bool>;
                                  { state.test(derived_atom) } -> std::same_as<bool>;
                                  { state.value(variable_view) } -> std::same_as<formalism::FDRValue>;
                                  { state.value(static_fterm_view) } -> std::same_as<ygg::float_t>;
                                  { state.value(fluent_fterm_view) } -> std::same_as<ygg::float_t>;
                                  { state.test(static_atom_view) } -> std::same_as<bool>;
                                  { state.test(fluent_atom_view) } -> std::same_as<bool>;
                                  { state.test(derived_atom_view) } -> std::same_as<bool>;
                                  { state.get_atoms(static_predicate) } -> AtomViewRangeConcept<formalism::StaticTag>;
                                  { state.get_atoms(fluent_predicate) } -> AtomViewRangeConcept<formalism::FluentTag>;
                                  { state.get_atoms(derived_predicate) } -> AtomViewRangeConcept<formalism::DerivedTag>;
                              };

}

#endif
