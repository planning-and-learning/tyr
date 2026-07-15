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

#include "tyr/formalism/planning/repository.hpp"

#include "matching.hpp"
#include "normalization.hpp"
#include "proof.hpp"
#include "utils.hpp"

#include <algorithm>
#include <optional>
#include <vector>

namespace tyr::formalism::planning::invariant
{
namespace
{

using PositionMapping = std::vector<std::pair<size_t, std::optional<ParameterIndex>>>;

ygg::Map<ParameterIndex, ygg::Data<Term>>
get_parameters_from_part(const MutableAtom<FluentTag>& part, const MutableAtom<FluentTag>& literal, size_t num_rigid_variables)
{
    ygg::Map<ParameterIndex, ygg::Data<Term>> result;

    for (size_t pos = 0; pos < part.terms.size(); ++pos)
    {
        std::visit(
            [&](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, ParameterIndex>)
                {
                    if (static_cast<ygg::uint_t>(arg) < num_rigid_variables)
                        result.emplace(arg, literal.terms[pos]);
                }
            },
            part.terms[pos].value);
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

void instantiate_factored_mapping_rec(const std::vector<std::pair<std::vector<size_t>, std::vector<std::optional<ParameterIndex>>>>& groups,
                                      size_t group_index,
                                      PositionMapping& current,
                                      std::vector<PositionMapping>& out)
{
    if (group_index == groups.size())
    {
        out.push_back(current);
        return;
    }

    const auto& [positions, images] = groups[group_index];

    auto perm = images;
    std::sort(perm.begin(), perm.end(), optional_parameter_less);

    do
    {
        for (size_t i = 0; i < positions.size(); ++i)
            current.push_back({ positions[i], perm[i] });

        instantiate_factored_mapping_rec(groups, group_index + 1, current, out);

        for (size_t i = 0; i < positions.size(); ++i)
            current.pop_back();
    } while (std::next_permutation(perm.begin(), perm.end(), optional_parameter_less));
}

std::vector<PositionMapping> instantiate_factored_mapping(const std::vector<std::pair<std::vector<size_t>, std::vector<std::optional<ParameterIndex>>>>& groups)
{
    std::vector<PositionMapping> result;
    PositionMapping current;
    instantiate_factored_mapping_rec(groups, 0, current, result);
    return result;
}

std::vector<PositionMapping> possible_mappings(const MutableAtom<FluentTag>& part,
                                               const MutableAtom<FluentTag>& own_literal,
                                               const MutableAtom<FluentTag>& other_literal,
                                               size_t num_rigid_variables)
{
    size_t allowed_omissions = other_literal.terms.size() - part_arity(part, num_rigid_variables);
    if (allowed_omissions > 1)
        return {};

    const auto own_parameters = get_parameters_from_part(part, own_literal, num_rigid_variables);

    ygg::Map<ygg::Data<Term>, std::vector<ParameterIndex>> ownarg_to_invariant_parameters;
    for (const auto& [inv_param, term] : own_parameters)
        ownarg_to_invariant_parameters[term].push_back(inv_param);

    ygg::Map<ygg::Data<Term>, std::vector<size_t>> other_arg_to_positions;
    for (size_t pos = 0; pos < other_literal.terms.size(); ++pos)
        other_arg_to_positions[other_literal.terms[pos]].push_back(pos);

    std::vector<std::pair<std::vector<size_t>, std::vector<std::optional<ParameterIndex>>>> factored_mapping;

    for (const auto& [term, other_positions] : other_arg_to_positions)
    {
        auto inv_params = ownarg_to_invariant_parameters[term];
        const int len_diff = static_cast<int>(inv_params.size()) - static_cast<int>(other_positions.size());

        if (len_diff >= 1 || len_diff <= -2 || (len_diff == -1 && !allowed_omissions))
            return {};

        std::vector<std::optional<ParameterIndex>> images;
        images.reserve(inv_params.size() + (len_diff != 0 ? 1 : 0));

        for (const auto& p : inv_params)
            images.push_back(p);

        if (len_diff != 0)
        {
            images.push_back(std::nullopt);
            allowed_omissions = 0;
        }

        factored_mapping.push_back({ other_positions, std::move(images) });
    }

    return instantiate_factored_mapping(factored_mapping);
}

MutableAtomList<FluentTag> possible_matches(const MutableAtom<FluentTag>& part,
                                            const MutableAtom<FluentTag>& own_literal,
                                            const MutableAtom<FluentTag>& other_literal,
                                            size_t num_rigid_variables)
{
    MutableAtomList<FluentTag> result;

    for (const auto& mapping : possible_mappings(part, own_literal, other_literal, num_rigid_variables))
    {
        std::vector<ygg::Data<Term>> args(other_literal.terms.size(), ygg::Data<Term>(ParameterIndex(num_rigid_variables)));

        for (const auto& [other_pos, inv_var] : mapping)
            args[other_pos] = ygg::Data<Term>(inv_var.value_or(ParameterIndex(num_rigid_variables)));

        result.push_back(MutableAtom<FluentTag>(other_literal.predicate, std::move(args)));
    }

    return result;
}
}  // namespace

InvariantList refine_candidate(const Invariant& inv, const Threat& threat, const MutableActionList& ops, PredicateListView<FluentTag>)
{
    InvariantList result;

    const auto& op = ops[threat.op_index];
    const auto& effect = op.effects[threat.effect_index];
    const auto& add_literal = effect.effect.literals[threat.add_index];
    assert(add_literal.polarity);
    const auto& add_atom = add_literal.atom;

    const MutableAtom<FluentTag>* part = find_part(inv, add_atom.predicate);
    if (part == nullptr)
        return result;

    for (const auto& conj_effect : op.effects)
    {
        for (const auto& literal : conj_effect.effect.literals)
        {
            if (literal.polarity)
                continue;

            const auto& del_atom = literal.atom;

            if (inv.predicates.contains(del_atom.predicate))
                continue;

            for (auto phi_prime : possible_matches(*part, add_atom, del_atom, inv.num_rigid_variables))
            {
                if (covers(inv, phi_prime))
                    continue;

                const size_t new_num_counted_variables =
                    std::max(inv.num_counted_variables, atom_uses_counted(phi_prime, inv.num_rigid_variables) ? size_t(1) : size_t(0));

                auto atoms = inv.atoms;
                atoms.push_back(phi_prime);

                auto refined = Invariant(inv.num_rigid_variables, new_num_counted_variables, std::move(atoms));
                normalize_invariant(refined);

                if (!is_add_effect_unbalanced(op, effect, add_atom, refined))
                    result.push_back(std::move(refined));
            }
        }
    }

    return result;
}

InvariantList make_initial_candidates(PredicateListView<FluentTag> predicates)
{
    InvariantList result;

    for (const auto predicate : predicates)
    {
        const auto arity = static_cast<size_t>(predicate.get_arity());

        result.emplace_back(arity, 0, MutableAtomList<FluentTag> { make_initial_atom(predicate, arity) });

        for (size_t counted_position = 0; counted_position < arity; ++counted_position)
            result.emplace_back(arity - 1, 1, MutableAtomList<FluentTag> { make_initial_atom(predicate, counted_position) });
    }

    return result;
}

}  // namespace tyr::formalism::planning::invariant
