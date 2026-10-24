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
#include <utility>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::planning
{
/**
 * IterableStateConcept
 */

template<class R, class Tag>
concept AtomRangeConcept =
    std::ranges::input_range<R> && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, ygg::Index<formalism::planning::Atom<GroundTag, Tag>>>;

template<class R, class Tag>
concept FactRangeConcept =
    std::ranges::input_range<R> && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, ygg::Data<formalism::planning::FDRFact<Tag>>>;

template<class R, class Tag>
concept FunctionTermValueRangeConcept =
    std::ranges::input_range<R>
    && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, std::pair<ygg::Index<formalism::planning::FunctionTerm<GroundTag, Tag>>, ygg::float_t>>;

template<typename T>
concept IterableStateConcept = requires(const std::remove_reference_t<T>& cs) {
    { cs.template get_atoms<formalism::StaticTag>() } -> AtomRangeConcept<formalism::StaticTag>;
    { cs.get_fluent_facts() } -> FactRangeConcept<formalism::FluentTag>;
    { cs.template get_atoms<formalism::FluentTag>() } -> AtomRangeConcept<formalism::FluentTag>;
    { cs.template get_atoms<formalism::DerivedTag>() } -> AtomRangeConcept<formalism::DerivedTag>;
    { cs.template get_fterm_values<formalism::StaticTag>() } -> FunctionTermValueRangeConcept<formalism::StaticTag>;
    { cs.template get_fterm_values<formalism::FluentTag>() } -> FunctionTermValueRangeConcept<formalism::FluentTag>;
};

/**
 * IterableViewStateConcept
 */

template<class R, class Tag>
concept AtomViewRangeConcept =
    std::ranges::input_range<R> && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, formalism::planning::AtomView<GroundTag, Tag>>;

template<class R, class Tag>
concept FactViewRangeConcept =
    std::ranges::input_range<R> && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, formalism::planning::FDRFactView<Tag>>;

template<class R, class Tag>
concept FunctionTermViewValueRangeConcept =
    std::ranges::input_range<R>
    && std::same_as<std::remove_cvref_t<std::ranges::range_value_t<R>>, formalism::planning::FunctionTermViewValuePair<GroundTag, Tag>>;

template<typename T>
concept IterableViewStateConcept = requires(const std::remove_reference_t<T>& cs) {
    { cs.template get_atoms_view<formalism::StaticTag>() } -> AtomViewRangeConcept<formalism::StaticTag>;
    { cs.get_fluent_facts_view() } -> FactViewRangeConcept<formalism::FluentTag>;
    { cs.template get_atoms_view<formalism::FluentTag>() } -> AtomViewRangeConcept<formalism::FluentTag>;
    { cs.template get_atoms_view<formalism::DerivedTag>() } -> AtomViewRangeConcept<formalism::DerivedTag>;
    { cs.template get_fterm_values_view<formalism::StaticTag>() } -> FunctionTermViewValueRangeConcept<formalism::StaticTag>;
    { cs.template get_fterm_values_view<formalism::FluentTag>() } -> FunctionTermViewValueRangeConcept<formalism::FluentTag>;
};

/// State contents and task context, accepting values and references without requiring repository identity.
template<typename T, typename Kind>
concept StateViewConcept = TaskKind<Kind> && IterableStateConcept<T> && IterableViewStateConcept<T>
                           && requires(const std::remove_reference_t<T>& state,
                                       ygg::Index<formalism::planning::FDRVariable<formalism::FluentTag>> variable,
                                       ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::StaticTag>> static_fterm,
                                       ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>> fluent_fterm,
                                       ygg::Index<formalism::planning::Atom<GroundTag, formalism::StaticTag>> static_atom,
                                       ygg::Index<formalism::planning::Atom<GroundTag, formalism::DerivedTag>> derived_atom,
                                       formalism::planning::FDRVariableView<formalism::FluentTag> variable_view,
                                       formalism::planning::FunctionTermView<GroundTag, formalism::StaticTag> static_fterm_view,
                                       formalism::planning::FunctionTermView<GroundTag, formalism::FluentTag> fluent_fterm_view,
                                       formalism::planning::AtomView<GroundTag, formalism::StaticTag> static_atom_view,
                                       formalism::planning::AtomView<GroundTag, formalism::FluentTag> fluent_atom_view,
                                       formalism::planning::AtomView<GroundTag, formalism::DerivedTag> derived_atom_view,
                                       formalism::planning::PredicateView<formalism::StaticTag> static_predicate,
                                       formalism::planning::PredicateView<formalism::FluentTag> fluent_predicate,
                                       formalism::planning::PredicateView<formalism::DerivedTag> derived_predicate) {
                                  { state.get_task() } -> std::same_as<const Task<Kind>&>;
                                  { state.get_formalism_repository() } -> std::same_as<const formalism::planning::RepositoryPtr&>;
                                  { state.get(variable) } -> std::same_as<formalism::planning::FDRValue>;
                                  { state.get(static_fterm) } -> std::same_as<ygg::float_t>;
                                  { state.get(fluent_fterm) } -> std::same_as<ygg::float_t>;
                                  { state.test(static_atom) } -> std::same_as<bool>;
                                  { state.test(derived_atom) } -> std::same_as<bool>;
                                  { state.get(variable_view) } -> std::same_as<formalism::planning::FDRValue>;
                                  { state.get(static_fterm_view) } -> std::same_as<ygg::float_t>;
                                  { state.get(fluent_fterm_view) } -> std::same_as<ygg::float_t>;
                                  { state.test(static_atom_view) } -> std::same_as<bool>;
                                  { state.test(fluent_atom_view) } -> std::same_as<bool>;
                                  { state.test(derived_atom_view) } -> std::same_as<bool>;
                                  { state.get_atoms_view(static_predicate) } -> AtomViewRangeConcept<formalism::StaticTag>;
                                  { state.get_atoms_view(fluent_predicate) } -> AtomViewRangeConcept<formalism::FluentTag>;
                                  { state.get_atoms_view(derived_predicate) } -> AtomViewRangeConcept<formalism::DerivedTag>;
                              };

}

#endif
