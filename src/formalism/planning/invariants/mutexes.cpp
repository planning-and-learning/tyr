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

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::formalism::planning::invariant
{
namespace
{

/// The objects `atom` binds the rigid variables of `part` to, or nullopt if it does not match the part: same arity,
/// equal constants, repeated variables bound consistently, and every rigid variable bound.
std::optional<urd::Store<urd::sequence<urd::index<Object>>>>
extract_rigid_values(InvariantView inv, AtomView<LiftedTag, FluentTag> part, AtomView<GroundTag, FluentTag> atom)
{
    const auto terms = part.get_terms();
    const auto objects = atom.get_objects();
    if (terms.size() != objects.size())
        return std::nullopt;

    auto binding = urd::Store<urd::sequence<std::optional<urd::index<Object>>>>(inv.get_num_rigid_variables() + inv.get_num_counted_variables());
    for (std::size_t position = 0; position < terms.size(); ++position)
    {
        const auto object = objects[static_cast<std::ptrdiff_t>(position)].index();
        const bool matches = std::visit(
            [&]<typename T>(const T& arg) -> bool
            {
                if constexpr (std::is_same_v<T, ParameterIndex>)
                {
                    auto& bound = binding[arg.value];
                    if (bound.has_value() && *bound != object)
                        return false;
                    bound = object;
                    return true;
                }
                else if constexpr (std::is_same_v<T, urd::index<Object>>)
                {
                    return arg == object;
                }
                else
                {
                    static_assert(false, "Unhandled case");
                }
            },
            urd::to_stored(terms[static_cast<std::ptrdiff_t>(position)]));
        if (!matches)
            return std::nullopt;
    }

    auto rigid_values = urd::Store<urd::sequence<urd::index<Object>>> {};
    rigid_values.reserve(inv.get_num_rigid_variables());
    for (std::size_t i = 0; i < inv.get_num_rigid_variables(); ++i)
    {
        const auto& value = binding[i];
        if (!value.has_value())
            return std::nullopt;
        rigid_values.push_back(*value);
    }
    return rigid_values;
}

/// One invariant instance: the reachable atoms whose rigid variables are bound to `rigid_values`.
struct InstanceGroup : urd::compared_by_identity<InstanceGroup>
{
    size_t inv_index;
    urd::Store<urd::sequence<urd::index<Object>>> rigid_values;
    AtomViewList<GroundTag, FluentTag> atoms;
    size_t num_initial_atoms = 0;

    InstanceGroup(size_t inv_index_, urd::Store<urd::sequence<urd::index<Object>>> rigid_values_) :
        inv_index(inv_index_),
        rigid_values(std::move(rigid_values_)),
        atoms()
    {
    }

    static constexpr auto identifying_members() noexcept
    { return urd::Members<&InstanceGroup::inv_index, &InstanceGroup::rigid_values, &InstanceGroup::atoms> {}; }
};

struct GroupKey : urd::compared_by_identity<GroupKey>
{
    size_t invariant_index;
    urd::Store<urd::sequence<urd::index<Object>>> rigid_values;

    GroupKey(size_t invariant_index_, urd::Store<urd::sequence<urd::index<Object>>> rigid_values_) :
        invariant_index(invariant_index_),
        rigid_values(std::move(rigid_values_))
    {
    }

    static constexpr auto identifying_members() noexcept { return urd::Members<&GroupKey::invariant_index, &GroupKey::rigid_values> {}; }
};

bool structural_less(AtomView<GroundTag, FluentTag> lhs, AtomView<GroundTag, FluentTag> rhs)
{
    const auto lhs_key = std::tuple(lhs.get_predicate(), lhs.get_objects());
    const auto rhs_key = std::tuple(rhs.get_predicate(), rhs.get_objects());

    return urd::Less<std::remove_cvref_t<decltype(lhs_key)>> {}(lhs_key, rhs_key);
}

bool structural_less(const AtomViewList<GroundTag, FluentTag>& lhs, const AtomViewList<GroundTag, FluentTag>& rhs)
{
    return std::lexicographical_compare(lhs.begin(),
                                        lhs.end(),
                                        rhs.begin(),
                                        rhs.end(),
                                        [](const auto lhs_atom, const auto rhs_atom) { return structural_less(lhs_atom, rhs_atom); });
}

using AtomSet = urd::Store<urd::set<urd::index<Atom<GroundTag, FluentTag>>>, urd::dense>;

AtomViewList<GroundTag, FluentTag> uncovered_atoms(const InstanceGroup& group, const AtomSet& covered)
{
    auto result = AtomViewList<GroundTag, FluentTag> {};
    result.reserve(group.atoms.size());

    for (const auto atom : group.atoms)
        if (!covered.contains(atom.index()))
            result.push_back(atom);

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

bool uncovered_structural_less(const InstanceGroup& lhs, const InstanceGroup& rhs, const AtomSet& covered)
{
    const auto lhs_atoms = uncovered_atoms(lhs, covered);
    const auto rhs_atoms = uncovered_atoms(rhs, covered);
    if (structural_less(lhs_atoms, rhs_atoms) || structural_less(rhs_atoms, lhs_atoms))
        return structural_less(lhs_atoms, rhs_atoms);

    return deterministic_group_less(lhs, rhs);
}

/// Every instance of every invariant with at least one reachable atom, in one pass over the atoms. An atom joins the
/// instance its rigid values determine; parts of one invariant have distinct predicates, so instances do not overlap.
urd::Store<urd::sequence<InstanceGroup>> instantiate_groups(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                                            const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                                            const InvariantViewList& invariants)
{
    auto is_initial = AtomSet {};
    for (const auto atom : initial_atoms)
    {
        if (static_cast<ygg::uint_t>(atom.index().value) >= all_atoms.size())
            throw std::invalid_argument("initial_atoms must be among all_atoms");
        is_initial.insert(atom.index());
    }

    auto predicates = urd::Store<urd::sequence<urd::Store<urd::set<PredicateView<FluentTag>>>>>(invariants.size());
    for (size_t inv_index = 0; inv_index < invariants.size(); ++inv_index)
        for (const auto part : invariants[inv_index].get_atoms())
            predicates[inv_index].insert(part.get_predicate());

    auto groups = urd::Store<urd::sequence<InstanceGroup>> {};
    auto group_index = urd::Store<urd::map<GroupKey, size_t>, urd::sorted> {};

    for (const auto atom : all_atoms)
    {
        for (size_t inv_index = 0; inv_index < invariants.size(); ++inv_index)
        {
            const auto inv = invariants[inv_index];
            if (!predicates[inv_index].contains(atom.get_predicate()))
                continue;

            for (const auto part : inv.get_atoms())
            {
                if (part.get_predicate() != atom.get_predicate())
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
                if (is_initial.contains(atom.index()))
                    ++group.num_initial_atoms;
            }
        }
    }

    for (auto& group : groups)
        std::sort(group.atoms.begin(), group.atoms.end(), [](const auto lhs, const auto rhs) { return structural_less(lhs, rhs); });

    return groups;
}

size_t compute_uncovered_coverage(const InstanceGroup& group, const AtomSet& covered)
{
    return std::ranges::count_if(group.atoms, [&](const auto atom) { return !covered.contains(atom.index()); });
}

std::optional<size_t> select_best_group(const urd::Store<urd::sequence<InstanceGroup>>& groups, const AtomSet& covered)
{
    std::optional<size_t> best_group_index;
    size_t best_coverage = 0;

    for (size_t i = 0; i < groups.size(); ++i)
    {
        const auto coverage = compute_uncovered_coverage(groups[i], covered);
        if (coverage > best_coverage
            || (coverage == best_coverage && best_group_index.has_value() && uncovered_structural_less(groups[i], groups[*best_group_index], covered)))
        {
            best_group_index = i;
            best_coverage = coverage;
        }
    }

    return best_group_index;
}

void mark_group_covered(const AtomViewList<GroundTag, FluentTag>& group, AtomSet& covered)
{
    for (const auto atom : group)
        covered.insert(atom.index());
}

MutexGroup make_single_atom_group(AtomView<GroundTag, FluentTag> atom) { return MutexGroup { std::nullopt, {}, { atom } }; }

/// The atoms of `group`'s instance, with its invariant and rigid objects.
MutexGroup make_group(const InvariantViewList& invariants, const InstanceGroup& group, AtomViewList<GroundTag, FluentTag> atoms)
{
    auto objects = ObjectViewList {};
    objects.reserve(group.rigid_values.size());
    for (const auto object : group.rigid_values)
        objects.push_back(atoms.front().context().view(object));
    return MutexGroup { invariants[group.inv_index], std::move(objects), std::move(atoms) };
}

void add_single_atom_groups(const AtomViewList<GroundTag, FluentTag>& all_atoms, const AtomSet& covered, urd::Store<urd::sequence<MutexGroup>>& result)
{
    for (const auto atom : all_atoms)
        if (!covered.contains(atom.index()))
            result.push_back(make_single_atom_group(atom));
}

urd::Store<urd::sequence<MutexGroup>> choose_groups_greedily(const InvariantViewList& invariants,
                                                             const urd::Store<urd::sequence<InstanceGroup>>& groups,
                                                             const AtomViewList<GroundTag, FluentTag>& all_atoms)
{
    auto covered = AtomSet {};

    urd::Store<urd::sequence<MutexGroup>> result;
    result.reserve(groups.size() + all_atoms.size());

    while (covered.size() < all_atoms.size())
    {
        const auto best_idx = select_best_group(groups, covered);
        if (!best_idx.has_value())
            break;

        auto selected_group = uncovered_atoms(groups[*best_idx], covered);
        if (selected_group.empty())
            break;

        mark_group_covered(selected_group, covered);
        result.push_back(make_group(invariants, groups[*best_idx], std::move(selected_group)));
    }

    add_single_atom_groups(all_atoms, covered, result);

    return result;
}

/// The covered sets are keyed by atom index, so all_atoms must hold exactly the atoms 0, 1, ... of its repository.
void check_atoms_are_indexed_densely(const AtomViewList<GroundTag, FluentTag>& all_atoms)
{
    for (ygg::uint_t i = 0; i < all_atoms.size(); ++i)
        if (static_cast<ygg::uint_t>(all_atoms[i].index().value) != i)
            throw std::invalid_argument("all_atoms must be the fluent atoms ordered by index, without gaps");
}

}  // namespace

urd::Store<urd::sequence<MutexGroup>> compute_mutex_groups(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                                           const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                                           const InvariantViewList& invariants)
{
    check_atoms_are_indexed_densely(all_atoms);

    auto groups = instantiate_groups(initial_atoms, all_atoms, invariants);
    // Candidates are the instances with exactly one true initial atom.
    erase_if(groups, [](const InstanceGroup& group) { return group.num_initial_atoms != 1; });
    std::sort(groups.begin(), groups.end(), deterministic_group_less);

    return choose_groups_greedily(invariants, groups, all_atoms);
}

urd::Store<urd::sequence<MutexGroup>> compute_mutex_groups_in_order(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                                                    const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                                                    const InvariantViewList& invariant_order)
{
    check_atoms_are_indexed_densely(all_atoms);

    auto groups = instantiate_groups(initial_atoms, all_atoms, invariant_order);
    // The invariant index is the position in the order, so this visits the invariants in order.
    std::sort(groups.begin(), groups.end(), deterministic_group_less);

    urd::Store<urd::sequence<MutexGroup>> result;
    result.reserve(groups.size() + all_atoms.size());

    auto covered = AtomSet {};

    for (const auto& group : groups)
    {
        auto atoms = uncovered_atoms(group, covered);
        if (atoms.empty())
            continue;

        mark_group_covered(atoms, covered);

        if (group.num_initial_atoms > 1)
        {
            // The invariant does not hold for this instance, and not for any subset of it either: its atoms stay
            // single instead of falling through to later invariants, which would make the structure task-dependent.
            for (const auto atom : atoms)
                result.push_back(make_single_atom_group(atom));
            continue;
        }

        result.push_back(make_group(invariant_order, group, std::move(atoms)));
    }

    add_single_atom_groups(all_atoms, covered, result);

    return result;
}
urd::Store<urd::sequence<MutexGroup>> compute_all_mutex_groups(const AtomViewList<GroundTag, FluentTag>& initial_atoms,
                                                               const AtomViewList<GroundTag, FluentTag>& all_atoms,
                                                               const InvariantViewList& invariants)
{
    check_atoms_are_indexed_densely(all_atoms);

    auto groups = instantiate_groups(initial_atoms, all_atoms, invariants);
    erase_if(groups, [](const InstanceGroup& group) { return group.num_initial_atoms > 1 || group.atoms.size() < 2; });
    std::sort(groups.begin(), groups.end(), deterministic_group_less);

    auto result = urd::Store<urd::sequence<MutexGroup>> {};
    result.reserve(groups.size());
    for (auto& group : groups)
        result.push_back(make_group(invariants, group, std::move(group.atoms)));
    return result;
}
}
