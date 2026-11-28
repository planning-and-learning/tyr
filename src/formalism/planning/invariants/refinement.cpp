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

#include "refinement.hpp"

#include "matching.hpp"
#include "normalization.hpp"
#include "proof.hpp"
#include "tyr/declarations.hpp"
#include "tyr/formalism/atom_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_effects.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/invariant_builder.hpp"
#include "tyr/formalism/repository.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <optional>
#include <urd/elements/clone.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <urd/tags.hpp>
#include <utility>
#include <variant>
#include <vector>

namespace tyr::formalism::invariant
{
namespace
{

using PositionMapping = urd::Store<urd::sequence<std::pair<size_t, std::optional<ParameterIndex>>>>;

urd::Store<urd::map<ParameterIndex, Term>, urd::sorted>
get_parameters_from_part(const urd::data<Atom<LiftedTag, FluentTag>>& part, const urd::data<Atom<LiftedTag, FluentTag>>& literal, size_t num_rigid_variables)
{
    urd::Store<urd::map<ParameterIndex, Term>, urd::sorted> result;

    for (size_t pos = 0; pos < part.terms.size(); ++pos)
    {
        std::visit(
            [&]<typename T>(const T& arg)
            {
                if constexpr (std::is_same_v<T, ParameterIndex>)
                {
                    if (arg.value < num_rigid_variables)
                        result.emplace(arg, literal.terms[pos]);
                }
            },
            part.terms[pos]);
    }

    return result;
}

bool optional_parameter_less(const std::optional<ParameterIndex>& a, const std::optional<ParameterIndex>& b)
{
    if (!a.has_value() && !b.has_value())
        return false;
    if (!a.has_value())
        return false;
    if (!b.has_value())
        return true;
    return *a < *b;
}

void instantiate_factored_mapping_rec(
    const urd::Store<urd::sequence<std::pair<urd::Store<urd::sequence<size_t>>, urd::Store<urd::sequence<std::optional<ParameterIndex>>>>>>& groups,
    size_t group_index,
    PositionMapping& current,
    urd::Store<urd::sequence<PositionMapping>>& out)
{
    if (group_index == groups.size())
    {
        out.push_back(current);
        return;
    }

    const auto& [positions, images] = groups[group_index];

    auto perm = images;
    std::sort(perm.begin(), perm.end(), optional_parameter_less);

    // Visits every permutation, starting with the sorted one.
    for (auto more = true; more; more = std::next_permutation(perm.begin(), perm.end(), optional_parameter_less))
    {
        for (size_t i = 0; i < positions.size(); ++i)
            current.emplace_back(positions[i], perm[i]);

        instantiate_factored_mapping_rec(groups, group_index + 1, current, out);

        for (size_t i = 0; i < positions.size(); ++i)
            current.pop_back();
    }
}

urd::Store<urd::sequence<PositionMapping>> instantiate_factored_mapping(
    const urd::Store<urd::sequence<std::pair<urd::Store<urd::sequence<size_t>>, urd::Store<urd::sequence<std::optional<ParameterIndex>>>>>>& groups)
{
    urd::Store<urd::sequence<PositionMapping>> result;
    PositionMapping current;
    instantiate_factored_mapping_rec(groups, 0, current, result);
    return result;
}

urd::Store<urd::sequence<PositionMapping>> possible_mappings(const urd::data<Atom<LiftedTag, FluentTag>>& part,
                                                             const urd::data<Atom<LiftedTag, FluentTag>>& own_literal,
                                                             const urd::data<Atom<LiftedTag, FluentTag>>& other_literal,
                                                             size_t num_rigid_variables)
{
    size_t allowed_omissions = other_literal.terms.size() - part_arity(part, num_rigid_variables);
    if (allowed_omissions > 1)
        return {};

    const auto own_parameters = get_parameters_from_part(part, own_literal, num_rigid_variables);

    urd::Store<urd::map<Term, urd::Store<urd::sequence<ParameterIndex>>>, urd::sorted> ownarg_to_invariant_parameters;
    for (const auto& [inv_param, term] : own_parameters)
        ownarg_to_invariant_parameters[term].push_back(inv_param);

    urd::Store<urd::map<Term, urd::Store<urd::sequence<size_t>>>, urd::sorted> other_arg_to_positions;
    for (size_t pos = 0; pos < other_literal.terms.size(); ++pos)
        other_arg_to_positions[other_literal.terms[pos]].push_back(pos);

    urd::Store<urd::sequence<std::pair<urd::Store<urd::sequence<size_t>>, urd::Store<urd::sequence<std::optional<ParameterIndex>>>>>> factored_mapping;

    for (const auto& [term, other_positions] : other_arg_to_positions)
    {
        auto inv_params = ownarg_to_invariant_parameters[term];
        const int len_diff = static_cast<int>(inv_params.size()) - static_cast<int>(other_positions.size());

        if (len_diff >= 1 || len_diff <= -2 || (len_diff == -1 && !allowed_omissions))
            return {};

        urd::Store<urd::sequence<std::optional<ParameterIndex>>> images;
        images.reserve(inv_params.size() + (len_diff != 0 ? 1 : 0));

        for (const auto& p : inv_params)
            images.emplace_back(p);

        if (len_diff != 0)
        {
            images.emplace_back(std::nullopt);
            allowed_omissions = 0;
        }

        factored_mapping.emplace_back(other_positions, std::move(images));
    }

    return instantiate_factored_mapping(factored_mapping);
}

std::vector<urd::data<Atom<LiftedTag, FluentTag>>> possible_matches(const urd::data<Atom<LiftedTag, FluentTag>>& part,
                                                                    const urd::data<Atom<LiftedTag, FluentTag>>& own_literal,
                                                                    const urd::data<Atom<LiftedTag, FluentTag>>& other_literal,
                                                                    size_t num_rigid_variables)
{
    auto result = std::vector<urd::data<Atom<LiftedTag, FluentTag>>>();

    for (const auto& mapping : possible_mappings(part, own_literal, other_literal, num_rigid_variables))
    {
        auto match = urd::data<Atom<LiftedTag, FluentTag>>();
        match.predicate = other_literal.predicate;
        match.terms.resize(other_literal.terms.size(), Term(ParameterIndex(num_rigid_variables)));

        for (const auto& [other_pos, inv_var] : mapping)
            match.terms[other_pos] = Term(inv_var.value_or(ParameterIndex(num_rigid_variables)));

        result.push_back(std::move(match));
    }

    return result;
}
}  // namespace

std::vector<urd::builder<Invariant, Repository>>
refine_candidate(const urd::builder<Invariant, Repository>& inv, const Threat& threat, const std::vector<ActionView<LiftedTag>>& ops, CandidateStorage& storage)
{
    auto result = std::vector<urd::builder<Invariant, Repository>>();

    const auto op = ops[threat.op_index];
    const auto add_literal = threat.effect.effect().literals()[static_cast<std::ptrdiff_t>(threat.add_index)];
    assert(add_literal.polarity());
    const auto& add_atom = *add_literal.atom().context()[add_literal.atom().index()];

    const auto* part = find_part(inv, add_atom.predicate);
    if (part == nullptr)
        return result;

    const auto visit_effect = [&](ConditionalEffectView<LiftedTag> conj_effect)
    {
        for (const auto literal : conj_effect.effect().literals())
        {
            if (literal.polarity() || is_overridden_delete(conj_effect, literal))
                continue;

            const auto& del_atom = *literal.atom().context()[literal.atom().index()];

            if (has_predicate(inv, del_atom.predicate))
                continue;

            for (auto& phi_prime : possible_matches(*part, add_atom, del_atom, inv.num_rigid_variables))
            {
                if (covers(inv, phi_prime))
                    continue;

                const std::size_t new_num_counted_variables =
                    std::max(static_cast<std::size_t>(inv.num_counted_variables),
                             atom_uses_counted(phi_prime, inv.num_rigid_variables) ? static_cast<std::size_t>(1) : static_cast<std::size_t>(0));

                auto atoms = std::vector<urd::data<Atom<LiftedTag, FluentTag>>>();
                for (const auto& existing : inv.atoms)
                    atoms.push_back(urd::clone(atom_of(existing)));
                atoms.push_back(std::move(phi_prime));

                auto refined = make_candidate(inv.num_rigid_variables, new_num_counted_variables, std::move(atoms), storage);
                normalize_invariant(refined);

                // Every match becomes a candidate, also one that does not balance this add effect yet: it can be refined
                // further, as in Fast Downward's refine_candidate.
                result.push_back(std::move(refined));
            }
        }
    };
    formalism::for_each_effect(op, [&](auto effect, formalism::OutcomePath /*path*/) { visit_effect(effect); });

    return result;
}

std::vector<urd::builder<Invariant, Repository>> make_initial_candidates(PredicateListView<FluentTag> predicates, CandidateStorage& storage)
{
    auto result = std::vector<urd::builder<Invariant, Repository>>();

    for (const auto predicate : predicates)
    {
        const auto arity = static_cast<std::size_t>(predicate.arity());

        auto atoms = std::vector<urd::data<Atom<LiftedTag, FluentTag>>>();
        atoms.push_back(make_initial_atom(predicate, arity));
        result.push_back(make_candidate(arity, 0, std::move(atoms), storage));

        for (std::size_t counted_position = 0; counted_position < arity; ++counted_position)
        {
            auto counted_atoms = std::vector<urd::data<Atom<LiftedTag, FluentTag>>>();
            counted_atoms.push_back(make_initial_atom(predicate, counted_position));
            result.push_back(make_candidate(arity - 1, 1, std::move(counted_atoms), storage));
        }
    }

    return result;
}

}  // namespace tyr::formalism::invariant
