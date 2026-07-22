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

#include "matching.hpp"

#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/unification/apply_substitution.hpp"
#include "tyr/formalism/unification/match_state.hpp"
#include "tyr/formalism/unification/match_term.hpp"
#include "tyr/formalism/unification/structure_traits.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"
#include "tyr/formalism/unification/substitution.hpp"

#include <algorithm>

namespace tyr::formalism::planning::invariant
{
namespace
{

using TermMatchState = tyr::formalism::unification::MatchState<ygg::Data<Term>>;
using ParameterRole = tyr::formalism::unification::ParameterRole;
using DefaultMatchPolicy = tyr::formalism::unification::DefaultMatchPolicy;

bool is_effect_local_parameter(ParameterIndex parameter, size_t num_action_variables) { return static_cast<ygg::uint_t>(parameter) >= num_action_variables; }

struct ActionAlignmentPolicy : tyr::formalism::unification::DefaultMatchPolicy
{
    size_t num_rigid_variables;
    size_t num_action_variables;

    template<typename T>
    bool match_parameter_parameter(ParameterIndex lhs, ParameterIndex rhs, tyr::formalism::unification::MatchState<T>& state) const
    {
        const bool lhs_is_counted = ygg::uint_t(lhs) >= num_rigid_variables;
        const bool rhs_is_action_parameter = ygg::uint_t(rhs) < num_action_variables;

        if (lhs_is_counted)
            return true;

        if (rhs_is_action_parameter)
            return state.sigma.assign_or_check(rhs, T(lhs));

        return lhs == rhs;
    }

    template<typename T>
    bool match_parameter_object(ParameterIndex lhs, const T&, tyr::formalism::unification::MatchState<T>&) const
    {
        return ygg::uint_t(lhs) >= num_rigid_variables;
    }

    template<typename T>
    bool match_object_parameter(const T& lhs, ParameterIndex rhs, tyr::formalism::unification::MatchState<T>& state) const
    {
        if (ygg::uint_t(rhs) < num_action_variables)
            return state.sigma.assign_or_check(rhs, lhs);

        return false;
    }
};

struct EffectCoverPolicy : tyr::formalism::unification::DefaultMatchPolicy
{
    size_t num_rigid_variables;
    size_t num_action_variables;

    template<typename T>
    bool match_parameter_parameter(ParameterIndex lhs, ParameterIndex rhs, tyr::formalism::unification::MatchState<T>& state) const
    {
        const bool lhs_is_counted = ygg::uint_t(lhs) >= num_rigid_variables;
        const bool rhs_is_effect_local = ygg::uint_t(rhs) >= num_action_variables;

        if (!lhs_is_counted)
        {
            if (rhs_is_effect_local)
                return state.sigma.assign_or_check(rhs, T(lhs));

            return lhs == rhs;
        }

        if (rhs_is_effect_local)
        {
            if (state.counted.is_bound(lhs))
                return state.sigma.assign_or_check(rhs, *state.counted[lhs]);

            return true;
        }

        if (state.counted.is_unbound(lhs))
            return state.counted.assign(lhs, T(rhs));

        return *state.counted[lhs] == T(rhs);
    }

    template<typename T>
    bool match_parameter_object(ParameterIndex lhs, const T& rhs, tyr::formalism::unification::MatchState<T>& state) const
    {
        if (ygg::uint_t(lhs) < num_rigid_variables)
            return false;

        if (state.counted.is_unbound(lhs))
            return state.counted.assign(lhs, rhs);

        return *state.counted[lhs] == rhs;
    }

    template<typename T>
    bool match_object_parameter(const T& lhs, ParameterIndex rhs, tyr::formalism::unification::MatchState<T>& state) const
    {
        if (ygg::uint_t(rhs) >= num_action_variables)
            return state.sigma.assign_or_check(rhs, lhs);

        return false;
    }
};

template<typename T, typename State, typename Matcher>
std::optional<State> match_structure(const T& pattern, const T& element, State state, Matcher&& matcher)
{
    const bool ok =
        tyr::formalism::unification::zip_terms(pattern, element, [&](const ygg::Data<Term>& lhs, const ygg::Data<Term>& rhs) { return matcher(lhs, rhs, state); });

    if (!ok)
        return std::nullopt;

    return state;
}

std::optional<InvariantSubstitution>
match_cover_against_atom(const Invariant& inv, const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& element)
{
    auto state = TermMatchState {
        .sigma = InvariantSubstitution::from_range(ParameterIndex { ygg::uint_t(inv.num_rigid_variables) }, inv.num_counted_variables),
        .counted = InvariantSubstitution {},
    };

    auto result = match_structure(pattern,
                                  element,
                                  std::move(state),
                                  [&](const ygg::Data<Term>& lhs, const ygg::Data<Term>& rhs, TermMatchState& st) -> bool
                                  { return tyr::formalism::unification::match_term(lhs, rhs, st, DefaultMatchPolicy {}); });

    if (!result.has_value())
        return std::nullopt;

    return std::move(result->sigma);
}

std::optional<EffectSubstitution>
match_effect_cover_against_atom(const Invariant& inv, const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& element, size_t num_action_variables)
{
    auto effect_parameters = std::vector<ParameterIndex> {};

    for (const auto& term : element.terms)
    {
        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, ParameterIndex>)
                {
                    if (is_effect_local_parameter(arg, num_action_variables))
                    {
                        if (std::find(effect_parameters.begin(), effect_parameters.end(), arg) == effect_parameters.end())
                            effect_parameters.push_back(arg);
                    }
                }
            },
            term.value);
    }

    auto state = TermMatchState {
        .sigma = EffectSubstitution(std::move(effect_parameters)),
        .counted = EffectSubstitution::from_range(ParameterIndex { ygg::uint_t(inv.num_rigid_variables) }, inv.num_counted_variables),
    };

    const auto policy = EffectCoverPolicy {
        .num_rigid_variables = inv.num_rigid_variables,
        .num_action_variables = num_action_variables,
    };

    auto result = match_structure(pattern,
                                  element,
                                  std::move(state),
                                  [&](const ygg::Data<Term>& lhs, const ygg::Data<Term>& rhs, TermMatchState& st) -> bool
                                  { return tyr::formalism::unification::match_term(lhs, rhs, st, policy); });

    if (!result.has_value())
        return std::nullopt;

    return std::move(result->sigma);
}

}  // namespace

bool covers(const Invariant& inv, const MutableAtom<FluentTag>& element)
{
    for (const auto& atom : inv.atoms)
    {
        if (match_cover_against_atom(inv, atom, element).has_value())
            return true;
    }

    return false;
}

std::optional<InvariantSubstitution>
match_invariant_against_ground_atom(const Invariant& inv, const MutableAtom<FluentTag>& pattern, const MutableAtom<FluentTag>& ground_atom)
{
    auto state = TermMatchState {
        .sigma = InvariantSubstitution::from_range(ParameterIndex { 0 }, inv.num_rigid_variables + inv.num_counted_variables),
        .counted = InvariantSubstitution {},
    };

    auto result = match_structure(pattern,
                                  ground_atom,
                                  std::move(state),
                                  [&](const ygg::Data<Term>& lhs, const ygg::Data<Term>& rhs, TermMatchState& st) -> bool
                                  { return tyr::formalism::unification::match_term(lhs, rhs, st, DefaultMatchPolicy {}); });

    if (!result.has_value())
        return std::nullopt;

    return std::move(result->sigma);
}

std::vector<ActionAlignment> enumerate_action_alignments(const Invariant& inv, const MutableAtom<FluentTag>& element, size_t num_action_variables)
{
    auto result = std::vector<ActionAlignment> {};

    const auto policy = ActionAlignmentPolicy {
        .num_rigid_variables = inv.num_rigid_variables,
        .num_action_variables = num_action_variables,
    };

    for (const auto& pattern : inv.atoms)
    {
        auto state = TermMatchState {
            .sigma = ActionSubstitution::from_range(ParameterIndex { 0 }, num_action_variables),
            .counted = ActionSubstitution {},
        };

        auto matched = match_structure(pattern,
                                       element,
                                       std::move(state),
                                       [&](const ygg::Data<Term>& lhs, const ygg::Data<Term>& rhs, TermMatchState& st) -> bool
                                       { return tyr::formalism::unification::match_term(lhs, rhs, st, policy); });

        if (matched.has_value())
        {
            result.push_back(ActionAlignment {
                .pattern = pattern,
                .sigma = std::move(matched->sigma),
            });
        }
    }

    return result;
}

std::vector<EffectSubstitution> enumerate_effect_renamings(const MutableConditionalEffect& effect,
                                                           const MutableAtom<FluentTag>& element,
                                                           const Invariant& inv,
                                                           const ActionSubstitution& sigma_op)
{
    auto result = std::vector<EffectSubstitution> {};

    const MutableAtom<FluentTag> partially_renamed = tyr::formalism::unification::apply_substitution(element, sigma_op);

    for (const auto& pattern : inv.atoms)
    {
        auto sigma_eff = match_effect_cover_against_atom(inv, pattern, partially_renamed, effect.num_parent_variables);

        if (!sigma_eff.has_value())
            continue;

        const MutableAtom<FluentTag> fully_renamed = tyr::formalism::unification::apply_substitution(partially_renamed, *sigma_eff);

        if (covers(inv, fully_renamed))
            result.push_back(std::move(*sigma_eff));
    }

    return result;
}

}  // namespace tyr::formalism::planning::invariant
