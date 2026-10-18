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

#include "matching.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/comparison.hpp>

namespace tyr::formalism::planning::invariant
{
namespace
{

std::optional<std::vector<ygg::Index<Object>>>
extract_rigid_values(const MutableInvariant& inv, const MutableAtom<FluentTag>& pattern, AtomView<GroundTag, FluentTag> atom)
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
            []<typename T>(const T& arg) -> std::optional<ygg::Index<Object>>
            {
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
            value->variant);

        if (!maybe_object.has_value())
            return std::nullopt;

        rigid_values.push_back(*maybe_object);
    }

    return rigid_values;
}

/// One invariant instance: the reachable atoms whose rigid variables are bound to `rigid_values`.
struct InstanceGroup : ygg::comparison::Mixin<InstanceGroup>
{
    size_t inv_index;
    std::vector<ygg::Index<Object>> rigid_values;
    AtomViewList<GroundTag, FluentTag> atoms;
    size_t num_initial_atoms = 0;

    InstanceGroup(size_t inv_index, std::vector<ygg::Index<Object>> rigid_values) : inv_index(inv_index), rigid_values(std::move(rigid_values)), atoms() {}

    auto identifying_members() const noexcept { return std::tie(inv_index, rigid_values, atoms); }
};

struct GroupKey : ygg::comparison::Mixin<GroupKey>
{
    size_t invariant_index;
    std::vector<ygg::Index<Object>> rigid_values;

    GroupKey(size_t invariant_index, std::vector<ygg::Index<Object>> rigid_values) : invariant_index(invariant_index), rigid_values(std::move(rigid_values)) {}

    auto identifying_members() const noexcept { return std::tie(invariant_index, rigid_values); }
};

bool structural_less(AtomView<GroundTag, FluentTag> lhs, AtomView<GroundTag, FluentTag> rhs)
{
    const auto lhs_key = std::tuple(lhs.get_predicate(), lhs.get_objects());
    const auto rhs_key = std::tuple(rhs.get_predicate(), rhs.get_objects());

    return ygg::Less<std::remove_cvref_t<decltype(lhs_key)>> {}(lhs_key, rhs_key);
}

bool structural_less(const AtomViewList<GroundTag, FluentTag>& lhs, const AtomViewList<GroundTag, FluentTag>& rhs)
{
    return std::lexicographical_compare(lhs.begin(),
                                        lhs.end(),
                                        rhs.begin(),
                                        rhs.end(),
                                        [](const auto lhs_atom, const auto rhs_atom) { return structural_less(lhs_atom, rhs_atom); });
}

AtomViewList<GroundTag, FluentTag> uncovered_atoms(const InstanceGroup& group, const std::vector<bool>& uncovered)
{
    auto result = AtomViewList<GroundTag, FluentTag> {};
    result.reserve(group.atoms.size());

    for (const auto atom : group.atoms)
    {
        const auto atom_index = ygg::uint_t(atom.get_index());
        if (uncovered[atom_index])
            result.push_back(atom);
    }

    return result;
}

bool deterministic_group_less(const InstanceGroup& lhs, const InstanceGroup& rhs)
{
    if (lhs.inv_index != rhs.inv_index)
        return lhs.inv_index < rhs.inv_index;

    if (lhs.rigid_values != rhs.rigid_values)
        return lhs.rigid_values < rhs.rigid_values;

    return structural_less(lhs.atoms, rhs.atoms);
}

bool uncovered_structural_less(const InstanceGroup& lhs, const InstanceGroup& rhs, const std::vector<bool>& uncovered)
{
    const auto lhs_atoms = uncovered_atoms(lhs, uncovered);
    const auto rhs_atoms = uncovered_atoms(rhs, uncovered);
    if (structural_less(lhs_atoms, rhs_atoms) || structural_less(rhs_atoms, lhs_atoms))
        return structural_less(lhs_atoms, rhs_atoms);

    return deterministic_group_less(lhs, rhs);
}

/// Every instance of every invariant with at least one reachable atom, in one pass over the atoms. An atom joins the
/// instance its rigid values determine; parts of one invariant have distinct predicates, so instances do not overlap.
std::vector<InstanceGroup> instantiate_groups(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                              const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                              const MutableInvariantList& invariants)
{
    auto is_initial = std::vector<bool>(all_atoms.size(), false);
    for (const auto atom : initial_atoms)
    {
        const auto i = ygg::uint_t(atom.get_index());
        if (i >= all_atoms.size())
            throw std::invalid_argument("initial_atoms must be among all_atoms");
        is_initial[i] = true;
    }

    auto groups = std::vector<InstanceGroup> {};
    auto group_index = ygg::Map<GroupKey, size_t> {};

    for (const auto atom : all_atoms)
    {
        for (size_t inv_index = 0; inv_index < invariants.size(); ++inv_index)
        {
            const auto& inv = invariants[inv_index];
            if (!inv.predicates.contains(atom.get_predicate()))
                continue;

            for (const auto& part : inv.atoms)
            {
                if (part.predicate != atom.get_predicate())
                    continue;

                auto rigid_values = extract_rigid_values(inv, part, atom);
                if (!rigid_values.has_value())
                    continue;

                auto [it, inserted] = group_index.emplace(GroupKey { inv_index, *rigid_values }, groups.size());
                if (inserted)
                    groups.emplace_back(inv_index, std::move(*rigid_values));

                auto& group = groups[it->second];
                if (!group.atoms.empty() && group.atoms.back() == atom)
                    continue;  // matched another part of the same instance

                group.atoms.push_back(atom);
                if (is_initial[ygg::uint_t(atom.get_index())])
                    ++group.num_initial_atoms;
            }
        }
    }

    for (auto& group : groups)
        std::sort(group.atoms.begin(), group.atoms.end(), [](const auto lhs, const auto rhs) { return structural_less(lhs, rhs); });

    return groups;
}

size_t compute_uncovered_coverage(const InstanceGroup& group, const std::vector<bool>& uncovered)
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

std::optional<size_t> select_best_group(const std::vector<InstanceGroup>& groups, const std::vector<bool>& uncovered)
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

void mark_group_covered(const AtomViewList<GroundTag, FluentTag>& group, std::vector<bool>& uncovered, size_t& num_uncovered)
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

void add_single_atom_groups(const AtomViewList<GroundTag, FluentTag>& all_atoms,
                            const std::vector<bool>& uncovered,
                            std::vector<AtomViewList<GroundTag, FluentTag>>& result)
{
    for (size_t pos = 0; pos < all_atoms.size(); ++pos)
    {
        if (uncovered[pos])
            result.push_back(AtomViewList<GroundTag, FluentTag> { all_atoms[pos] });
    }
}

std::vector<AtomViewList<GroundTag, FluentTag>> choose_groups_greedily(const std::vector<InstanceGroup>& groups,
                                                                       const AtomViewList<GroundTag, FluentTag>& all_atoms)
{
    std::vector<bool> uncovered(all_atoms.size(), true);
    size_t num_uncovered = all_atoms.size();

    std::vector<AtomViewList<GroundTag, FluentTag>> result;
    result.reserve(groups.size() + all_atoms.size());

    while (num_uncovered > 0)
    {
        const auto best_idx = select_best_group(groups, uncovered);
        if (!best_idx.has_value())
            break;

        auto selected_group = uncovered_atoms(groups[*best_idx], uncovered);
        if (selected_group.empty())
            break;

        mark_group_covered(selected_group, uncovered, num_uncovered);
        result.push_back(std::move(selected_group));
    }

    add_single_atom_groups(all_atoms, uncovered, result);

    return result;
}

/// The covers index bit vectors by atom index, so all_atoms must hold exactly the atoms 0, 1, ... of its repository.
void check_atoms_are_indexed_densely(const AtomViewList<GroundTag, FluentTag>& all_atoms)
{
    for (ygg::uint_t i = 0; i < all_atoms.size(); ++i)
        if (ygg::uint_t(all_atoms[i].get_index()) != i)
            throw std::invalid_argument("all_atoms must be the fluent atoms ordered by index, without gaps");
}

}  // namespace

std::vector<AtomViewList<GroundTag, FluentTag>> compute_mutex_groups(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                                                     const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                                                     const MutableInvariantList& invariants)
{
    check_atoms_are_indexed_densely(all_atoms);

    auto groups = instantiate_groups(initial_atoms, all_atoms, invariants);
    // Candidates are the instances with exactly one true initial atom.
    std::erase_if(groups, [](const InstanceGroup& group) { return group.num_initial_atoms != 1; });
    std::sort(groups.begin(), groups.end(), deterministic_group_less);

    return choose_groups_greedily(groups, all_atoms);
}

std::vector<AtomViewList<GroundTag, FluentTag>> compute_mutex_groups_in_order(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                                                              const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                                                              const MutableInvariantList& invariant_order)
{
    check_atoms_are_indexed_densely(all_atoms);

    auto groups = instantiate_groups(initial_atoms, all_atoms, invariant_order);
    // The invariant index is the position in the order, so this visits the invariants in order.
    std::sort(groups.begin(), groups.end(), deterministic_group_less);

    std::vector<AtomViewList<GroundTag, FluentTag>> result;
    result.reserve(groups.size() + all_atoms.size());

    std::vector<bool> uncovered(all_atoms.size(), true);
    size_t num_uncovered = all_atoms.size();

    for (const auto& group : groups)
    {
        auto atoms = uncovered_atoms(group, uncovered);
        if (atoms.empty())
            continue;

        mark_group_covered(atoms, uncovered, num_uncovered);

        if (group.num_initial_atoms > 1)
        {
            // The invariant does not hold for this instance, and not for any subset of it either: its atoms stay
            // single instead of falling through to later invariants, which would make the structure task-dependent.
            for (const auto atom : atoms)
                result.push_back(AtomViewList<GroundTag, FluentTag> { atom });
            continue;
        }

        result.push_back(std::move(atoms));
    }

    add_single_atom_groups(all_atoms, uncovered, result);

    return result;
}
std::vector<AtomViewList<GroundTag, FluentTag>> compute_all_mutex_groups(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                                                         const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                                                         const MutableInvariantList& invariants)
{
    check_atoms_are_indexed_densely(all_atoms);

    auto groups = instantiate_groups(initial_atoms, all_atoms, invariants);
    std::erase_if(groups, [](const InstanceGroup& group) { return group.num_initial_atoms > 1 || group.atoms.size() < 2; });
    std::sort(groups.begin(), groups.end(), deterministic_group_less);

    auto result = std::vector<AtomViewList<GroundTag, FluentTag>> {};
    result.reserve(groups.size());
    for (auto& group : groups)
        result.push_back(std::move(group.atoms));
    return result;
}
}
