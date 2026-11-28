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

#ifndef TYR_FORMALISM_CONJUNCTIVE_CONDITION_VIEW_HPP_
#define TYR_FORMALISM_CONJUNCTIVE_CONDITION_VIEW_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <concepts>
#include <fmt/base.h>
#include <ranges>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <utility>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace tyr::formalism::detail
{
/// The repository a view of a record (index_view, borrowed_view, ...: a view of T through C) reads through.
template<typename V>
struct ViewContext;

template<template<typename, typename, typename...> class View, typename T, typename C, typename... Rest>
struct ViewContext<View<T, C, Rest...>>
{
    using type = C;
};

/// A predicate of kind F through the repository a view V reads through.
template<typename V, typename F>
using PredicateViewOf = urd::index_view<Predicate<F>, typename ViewContext<V>::type>;
}  // namespace tyr::formalism::detail

namespace urd
{
/// The view of a conjunctive condition through either repository: the lifted condition is stored by both, the ground
/// one by planning's.
template<::tyr::TaskKind T, typename V>
struct view_mixin<::tyr::formalism::ConjunctiveCondition<T>, V>
{
private:
    [[nodiscard]] const V& self() const noexcept { return static_cast<const V&>(*this); }

public:
    auto variables() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::variables>(); }

    auto static_literals() const noexcept { return self().template member<&urd::record_of<V>::static_literals>(); }

    auto fluent_literals() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return self().template member<&urd::record_of<V>::fluent_literals>(); }

    auto derived_literals() const noexcept { return self().template member<&urd::record_of<V>::derived_literals>(); }

    auto positive_facts() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::positive_facts>(); }

    auto negative_facts() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return self().template member<&urd::record_of<V>::negative_facts>(); }

    auto numeric_constraints() const noexcept(std::same_as<T, ::tyr::GroundTag>)
    { return self().template member<&urd::record_of<V>::numeric_constraints>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::FactKind F>
    auto literals() const noexcept
    { return self().template member<::tyr::formalism::condition_literals_of<F, T>>(); }

    // user: one getter templated on the tag over the fields it chooses (next to the record), besides the plain getters.
    template<::tyr::formalism::PolarityKind F>
    auto facts() const noexcept
        requires std::same_as<T, ::tyr::GroundTag>
    { return self().template member<::tyr::formalism::condition_facts_of<F>>(); }

    auto arity() const noexcept
        requires std::same_as<T, ::tyr::LiftedTag>
    { return variables().size(); }

    /// Whether the condition has no literals and no numeric constraints, i.e., holds in every state.
    [[nodiscard]] bool empty() const
        requires std::same_as<T, ::tyr::LiftedTag>
    {
        return literals<::tyr::formalism::StaticTag>().empty() && literals<::tyr::formalism::FluentTag>().empty()
               && literals<::tyr::formalism::DerivedTag>().empty() && numeric_constraints().empty();
    }

    /// Projects represented atoms of the requested polarity, borrowing the repository.
    template<::tyr::formalism::FactKind F>
    auto atoms(bool polarity) const
    {
        if constexpr (std::same_as<T, ::tyr::GroundTag> && std::same_as<F, ::tyr::formalism::FluentTag>)
        {
            auto facts = polarity ? self().template facts<::tyr::formalism::PositiveTag>() : self().template facts<::tyr::formalism::NegativeTag>();
            return std::move(facts) | std::views::filter([](auto fact) { return fact.has_value(); })
                   | std::views::transform([](auto fact) { return *fact.atom(); });
        }
        else
        {
            return literals<F>() | std::views::filter([polarity](auto literal) { return literal.polarity() == polarity; })
                   | std::views::transform([](auto literal) { return literal.atom(); });
        }
    }

    template<::tyr::formalism::FactKind F>
    auto atoms(bool polarity, ::tyr::formalism::detail::PredicateViewOf<V, F> predicate) const
    {
        return atoms<F>(polarity) | std::views::filter([predicate](auto atom) { return atom.predicate() == predicate; });
    }

    static constexpr auto functions() noexcept
    {
        using Self = const view_mixin&;
        const auto atoms =
            urd::Functions<urd::function<"atoms",
                                         static_cast<decltype(std::declval<Self>().template atoms<::tyr::formalism::StaticTag>(
                                             true,
                                             std::declval<::tyr::formalism::detail::PredicateViewOf<V, ::tyr::formalism::StaticTag>>())) (
                                             view_mixin::*)(bool, ::tyr::formalism::detail::PredicateViewOf<V, ::tyr::formalism::StaticTag>) const>(
                                             &view_mixin::template atoms<::tyr::formalism::StaticTag>),
                                         urd::arg<"polarity">,
                                         urd::arg<"predicate">>,
                           urd::function<"atoms",
                                         static_cast<decltype(std::declval<Self>().template atoms<::tyr::formalism::FluentTag>(
                                             true,
                                             std::declval<::tyr::formalism::detail::PredicateViewOf<V, ::tyr::formalism::FluentTag>>())) (
                                             view_mixin::*)(bool, ::tyr::formalism::detail::PredicateViewOf<V, ::tyr::formalism::FluentTag>) const>(
                                             &view_mixin::template atoms<::tyr::formalism::FluentTag>),
                                         urd::arg<"polarity">,
                                         urd::arg<"predicate">>,
                           urd::function<"atoms",
                                         static_cast<decltype(std::declval<Self>().template atoms<::tyr::formalism::DerivedTag>(
                                             true,
                                             std::declval<::tyr::formalism::detail::PredicateViewOf<V, ::tyr::formalism::DerivedTag>>())) (
                                             view_mixin::*)(bool, ::tyr::formalism::detail::PredicateViewOf<V, ::tyr::formalism::DerivedTag>) const>(
                                             &view_mixin::template atoms<::tyr::formalism::DerivedTag>),
                                         urd::arg<"polarity">,
                                         urd::arg<"predicate">>> {};
        if constexpr (std::same_as<T, ::tyr::GroundTag>)
            return atoms;
        else
            return atoms + urd::Functions<urd::function<"arity", &view_mixin::arity>, urd::function<"empty", &view_mixin::empty>> {};
    }
};
}  // namespace urd

/// A condition: (and literals... constraints...), a ground one's facts as the literals they stand for.
template<tyr::TaskKind T>
struct fmt::formatter<urd::index_view<tyr::formalism::ConjunctiveCondition<T>, tyr::formalism::Repository>> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const urd::index_view<tyr::formalism::ConjunctiveCondition<T>, tyr::formalism::Repository>& value,
                                         fmt::format_context& context) const;
};

#endif
