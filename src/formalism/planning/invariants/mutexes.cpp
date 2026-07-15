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

#include "tyr/formalism/planning/invariants/mutexes.hpp"

#include "tyr/formalism/planning/repository.hpp"

#include "matching.hpp"

#include <algorithm>
#include <cassert>
#include <optional>
#include <tuple>
#include <vector>

namespace tyr::formalism::planning::invariant
{
namespace
{

std::optional<std::vector<ygg::Index<Object>>> extract_rigid_values(const Invariant& inv, const MutableAtom<FluentTag>& pattern, GroundAtomView<FluentTag> atom)
{
    const auto sigma = match_invariant_against_ground_atom(inv, pattern, MutableAtom<FluentTag>(atom));
    if (!sigma.has_value())
        return std::nullopt;

    std::vector<ygg::Index<Object>> rigid_values;
    rigid_values.reserve(inv.num_rigid_variables);

    for (size_t i = 0; i < inv.num_rigid_variables; ++i)
    {
        const auto& value = (*sigma)[ParameterIndex(i)];
        if (!value.has_value())
            return std::nullopt;

        const auto maybe_object = std::visit(
            [](auto&& arg) -> std::optional<ygg::Index<Object>>
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, ygg::Index<Object>>)
                {
                    return arg;
                }
                else if constexpr (std::is_same_v<T, ParameterIndex>)
                {
                    return std::nullopt;
                }
                else
                {
                    static_assert(ygg::dependent_false<T>::value, "Unhandled case");
                }
            },
            value->value);

        if (!maybe_object.has_value())
            return std::nullopt;

        rigid_values.push_back(*maybe_object);
    }

    return rigid_values;
}

bool instantiate_matches_ground_atom(const MutableAtom<FluentTag>& pattern,
                                     const std::vector<ygg::Index<Object>>& rigid_values,
                                     std::optional<ygg::Index<Object>> counted_value,
                                     GroundAtomView<FluentTag> ground_atom)
{
    if (!ygg::EqualTo<PredicateView<FluentTag>> {}(pattern.predicate, ground_atom.get_predicate()))
        return false;
    if (pattern.terms.size() != ground_atom.get_objects().size())
        return false;

    for (size_t pos = 0; pos < pattern.terms.size(); ++pos)
    {
        const auto object = ground_atom.get_objects()[pos].get_index();

        const bool ok = std::visit(
            [&](auto&& arg) -> bool
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, ParameterIndex>)
                {
                    const auto idx = static_cast<ygg::uint_t>(arg);
                    if (idx < rigid_values.size())
                        return rigid_values[idx] == object;

                    return counted_value.has_value() && *counted_value == object;
                }
                else if constexpr (std::is_same_v<T, ygg::Index<Object>>)
                {
                    return arg == object;
                }
                else
                {
                    static_assert(ygg::dependent_false<T>::value, "Unhandled case");
                }
            },
            pattern.terms[pos].value);

        if (!ok)
            return false;
    }

    return true;
}

bool initial_atom_matches_part(const Invariant& inv, const MutableAtom<FluentTag>& part, GroundAtomView<FluentTag> atom)
{
    return match_invariant_against_ground_atom(inv, part, MutableAtom<FluentTag>(atom)).has_value();
}

GroundAtomViewList<FluentTag>
instantiate_group(const Invariant& inv, const std::vector<ygg::Index<Object>>& rigid_values, const GroundAtomViewList<FluentTag>& all_atoms)
{
    assert(inv.num_counted_variables <= 1);

    GroundAtomViewList<FluentTag> result;
    std::vector<bool> seen(all_atoms.size(), false);

    for (const auto& pattern : inv.atoms)
    {
        std::optional<size_t> counted_position;

        for (size_t pos = 0; pos < pattern.terms.size(); ++pos)
        {
            std::visit(
                [&](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, ParameterIndex>)
                    {
                        if (static_cast<ygg::uint_t>(arg) >= inv.num_rigid_variables && !counted_position.has_value())
                            counted_position = pos;
                    }
                },
                pattern.terms[pos].value);

            if (counted_position.has_value())
                break;
        }

        for (const auto atom : all_atoms)
        {
            if (!ygg::EqualTo<PredicateView<FluentTag>> {}(pattern.predicate, atom.get_predicate()))
                continue;
            if (pattern.terms.size() != atom.get_objects().size())
                continue;

            std::optional<ygg::Index<Object>> counted_object = std::nullopt;
            if (counted_position.has_value())
                counted_object = atom.get_objects()[*counted_position].get_index();

            if (!instantiate_matches_ground_atom(pattern, rigid_values, counted_object, atom))
                continue;

            const auto i = ygg::uint_t(atom.get_index());
            if (!seen[i])
            {
                seen[i] = true;
                result.push_back(atom);
            }
        }
    }

    return result;
}

struct PrecomputedGroup
{
    size_t inv_index;
    std::vector<ygg::Index<Object>> rigid_values;
    GroundAtomViewList<FluentTag> atoms;

    auto identifying_members() const noexcept { return std::tie(inv_index, rigid_values, atoms); }
};

struct GroupKey
{
    size_t invariant_index;
    std::vector<ygg::Index<Object>> rigid_values;

    auto identifying_members() const noexcept { return std::tie(invariant_index, rigid_values); }
};

bool structural_less(GroundAtomView<FluentTag> lhs, GroundAtomView<FluentTag> rhs)
{
    const auto lhs_key = std::tuple(lhs.get_predicate(), lhs.get_objects());
    const auto rhs_key = std::tuple(rhs.get_predicate(), rhs.get_objects());

    return ygg::Less<std::remove_cvref_t<decltype(lhs_key)>> {}(lhs_key, rhs_key);
}

bool structural_less(const GroundAtomViewList<FluentTag>& lhs, const GroundAtomViewList<FluentTag>& rhs)
{
    return std::lexicographical_compare(lhs.begin(),
                                        lhs.end(),
                                        rhs.begin(),
                                        rhs.end(),
                                        [](const auto lhs_atom, const auto rhs_atom) { return structural_less(lhs_atom, rhs_atom); });
}

GroundAtomViewList<FluentTag> uncovered_atoms(const PrecomputedGroup& group, const std::vector<bool>& uncovered)
{
    auto result = GroundAtomViewList<FluentTag> {};
    result.reserve(group.atoms.size());

    for (const auto atom : group.atoms)
    {
        const auto atom_index = ygg::uint_t(atom.get_index());
        if (uncovered[atom_index])
            result.push_back(atom);
    }

    return result;
}

bool deterministic_group_less(const PrecomputedGroup& lhs, const PrecomputedGroup& rhs)
{
    if (lhs.inv_index != rhs.inv_index)
        return lhs.inv_index < rhs.inv_index;

    if (lhs.rigid_values != rhs.rigid_values)
        return lhs.rigid_values < rhs.rigid_values;

    return structural_less(lhs.atoms, rhs.atoms);
}

bool uncovered_structural_less(const PrecomputedGroup& lhs, const PrecomputedGroup& rhs, const std::vector<bool>& uncovered)
{
    const auto lhs_atoms = uncovered_atoms(lhs, uncovered);
    const auto rhs_atoms = uncovered_atoms(rhs, uncovered);
    if (structural_less(lhs_atoms, rhs_atoms) || structural_less(rhs_atoms, lhs_atoms))
        return structural_less(lhs_atoms, rhs_atoms);

    return deterministic_group_less(lhs, rhs);
}

std::vector<PrecomputedGroup>
precompute_groups(const GroundAtomViewList<FluentTag>& initial_atoms, const GroundAtomViewList<FluentTag>& all_atoms, const InvariantList& invariants)
{
    std::vector<PrecomputedGroup> groups;
    ygg::Set<GroupKey> seen_keys;

    for (size_t inv_index = 0; inv_index < invariants.size(); ++inv_index)
    {
        const auto& inv = invariants[inv_index];

        for (const auto atom : initial_atoms)
        {
            if (!inv.predicates.contains(atom.get_predicate()))
                continue;

            for (const auto& part : inv.atoms)
            {
                if (!ygg::EqualTo<PredicateView<FluentTag>> {}(part.predicate, atom.get_predicate()))
                    continue;
                if (!initial_atom_matches_part(inv, part, atom))
                    continue;

                auto rigid_values = extract_rigid_values(inv, part, atom);
                if (!rigid_values.has_value())
                    continue;

                GroupKey key { inv_index, *rigid_values };
                if (!seen_keys.insert(key).second)
                    continue;

                auto instantiated_group = instantiate_group(inv, *rigid_values, all_atoms);
                if (instantiated_group.empty())
                    continue;

                size_t initial_count = 0;
                for (const auto gatom : instantiated_group)
                {
                    if (std::find_if(initial_atoms.begin(),
                                     initial_atoms.end(),
                                     [&](const auto initial_atom) { return ygg::EqualTo<GroundAtomView<FluentTag>> {}(initial_atom, gatom); })
                        != initial_atoms.end())
                        ++initial_count;
                }

                if (initial_count > 1)
                    continue;

                std::sort(instantiated_group.begin(), instantiated_group.end(), [](const auto lhs, const auto rhs) { return structural_less(lhs, rhs); });

                groups.push_back(PrecomputedGroup {
                    .inv_index = inv_index,
                    .rigid_values = *rigid_values,
                    .atoms = std::move(instantiated_group),
                });
            }
        }
    }

    return groups;
}

size_t compute_uncovered_coverage(const PrecomputedGroup& group, const std::vector<bool>& uncovered)
{
    size_t coverage = 0;
    for (const auto atom : group.atoms)
    {
        const auto i = ygg::uint_t(atom.get_index());
        if (uncovered[i])
            ++coverage;
    }
    return coverage;
}

std::optional<size_t> select_best_group(const std::vector<PrecomputedGroup>& groups, const std::vector<bool>& uncovered)
{
    std::optional<size_t> best_group_index;
    size_t best_coverage = 0;

    for (size_t i = 0; i < groups.size(); ++i)
    {
        const auto coverage = compute_uncovered_coverage(groups[i], uncovered);
        if (coverage > best_coverage
            || (coverage == best_coverage && best_group_index.has_value() && uncovered_structural_less(groups[i], groups[*best_group_index], uncovered)))
        {
            best_group_index = i;
            best_coverage = coverage;
        }
    }

    return best_group_index;
}

GroundAtomViewList<FluentTag> build_uncovered_subgroup(const PrecomputedGroup& group, const std::vector<bool>& uncovered)
{
    GroundAtomViewList<FluentTag> result;
    result.reserve(group.atoms.size());

    for (const auto atom : group.atoms)
    {
        const auto i = ygg::uint_t(atom.get_index());
        if (uncovered[i])
            result.push_back(atom);
    }

    return result;
}

void mark_group_covered(const GroundAtomViewList<FluentTag>& group, std::vector<bool>& uncovered, size_t& num_uncovered)
{
    for (const auto atom : group)
    {
        const auto i = ygg::uint_t(atom.get_index());
        if (uncovered[i])
        {
            uncovered[i] = false;
            --num_uncovered;
        }
    }
}

std::vector<GroundAtomViewList<FluentTag>> choose_groups_greedily(const std::vector<PrecomputedGroup>& groups, const GroundAtomViewList<FluentTag>& all_atoms)
{
    std::vector<bool> uncovered(all_atoms.size(), true);
    size_t num_uncovered = all_atoms.size();

    std::vector<GroundAtomViewList<FluentTag>> result;
    result.reserve(groups.size() + all_atoms.size());

    while (num_uncovered > 0)
    {
        const auto best_idx = select_best_group(groups, uncovered);
        if (!best_idx.has_value())
            break;

        auto selected_group = build_uncovered_subgroup(groups[*best_idx], uncovered);
        if (selected_group.empty())
            break;

        result.push_back(selected_group);
        mark_group_covered(result.back(), uncovered, num_uncovered);
    }

    for (size_t pos = 0; pos < all_atoms.size(); ++pos)
    {
        if (uncovered[pos])
            result.push_back(GroundAtomViewList<FluentTag> { all_atoms[pos] });
    }

    return result;
}

}  // namespace

std::vector<GroundAtomViewList<FluentTag>>
compute_mutex_groups(const GroundAtomViewList<FluentTag>& initial_atoms, const GroundAtomViewList<FluentTag>& all_atoms, const InvariantList& invariants)
{
    for (ygg::uint_t i = 0; i < all_atoms.size(); ++i)
        assert(ygg::uint_t(all_atoms[i].get_index()) == i);

    auto groups = precompute_groups(initial_atoms, all_atoms, invariants);
    std::sort(groups.begin(), groups.end(), deterministic_group_less);

    return choose_groups_greedily(groups, all_atoms);
}

}
