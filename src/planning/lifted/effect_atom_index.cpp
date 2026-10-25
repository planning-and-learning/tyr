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

#include "effect_atom_index.hpp"

#include <algorithm>
#include <cassert>

namespace tyr::planning
{

EffectAtomIndex::EffectAtomIndex(const AppPredicateToActionMapping& predicate_to_action,
                                 const ConditionalEffectToPredicateMapping& effect_to_predicate,
                                 const datalog::PredicateFactSets<formalism::FluentTag>& facts,
                                 const formalism::datalog::Repository& repository) :
    m_effect_to_predicate(&effect_to_predicate),
    m_facts(&facts),
    m_repository(&repository)
{
    for (const auto& [predicate, action] : predicate_to_action)
        m_applicability_predicates.emplace(action, predicate);
}

void EffectAtomIndex::clear() noexcept
{
    for (auto& [_, groups] : m_groups)
        groups.current = false;
}

std::optional<std::span<const EffectAtomIndex::AtomIndex>> EffectAtomIndex::get_atoms(formalism::planning::ActionView<LiftedTag> action,
                                                                                      formalism::planning::ConditionalEffectView<LiftedTag> effect,
                                                                                      std::span<const ygg::Index<formalism::Object>> binding)
{
    const auto effect_predicate = m_effect_to_predicate->find(std::pair(action, effect));
    if (effect_predicate == m_effect_to_predicate->end())
        return std::nullopt;

    const auto applicability_predicate = m_applicability_predicates.at(action);
    auto& groups = m_groups[effect_predicate->second];
    if (!groups.current)
        rebuild(groups, effect_predicate->second, applicability_predicate, action.get_arity());

    const auto row = find_row(applicability_predicate, binding);
    if (!row || *row + 1 >= groups.offsets.size())
        return std::span<const AtomIndex> {};
    return std::span(groups.atoms).subspan(groups.offsets[*row], groups.offsets[*row + 1] - groups.offsets[*row]);
}

void EffectAtomIndex::rebuild(Groups& groups,
                              formalism::datalog::PredicateView<formalism::FluentTag> effect_predicate,
                              formalism::datalog::PredicateView<formalism::FluentTag> applicability_predicate,
                              size_t action_arity)
{
    const auto atoms = m_facts->get_sets()[ygg::uint_t(effect_predicate.get_index())].get_bindings();

    // The applicability row of each atom's action binding, which holds since the effect rule's body contains the precondition.
    groups.atom_rows.clear();
    for (const auto atom : atoms)
    {
        const auto objects = atom.get_objects();
        m_prefix.clear();
        for (size_t i = 0; i < action_arity; ++i)
            m_prefix.push_back(objects[i].get_index());
        const auto row = find_row(applicability_predicate, std::span<const ygg::Index<formalism::Object>>(m_prefix.data(), m_prefix.size()));
        assert(row && "An effect atom's action binding is applicable.");
        groups.atom_rows.push_back(*row);
    }

    // Counting sort by row: offsets[r + 1] counts the atoms of row r, then holds where they end.
    const auto num_rows = groups.atom_rows.empty() ? static_cast<ygg::uint_t>(0) : *std::ranges::max_element(groups.atom_rows) + 1;
    groups.offsets.assign(num_rows + 1, 0);
    for (const auto row : groups.atom_rows)
        ++groups.offsets[row + 1];
    for (size_t row = 0; row < num_rows; ++row)
        groups.offsets[row + 1] += groups.offsets[row];

    groups.atoms.resize(groups.atom_rows.size());
    auto i = static_cast<size_t>(0);
    for (const auto atom : atoms)
        groups.atoms[groups.offsets[groups.atom_rows[i++]]++] = atom.get_index();
    // Filling advanced each start to the next row's start; shift them back.
    for (auto row = num_rows; row > 0; --row)
        groups.offsets[row] = groups.offsets[row - 1];
    if (num_rows > 0)
        groups.offsets[0] = 0;

    groups.current = true;
}

std::optional<ygg::uint_t> EffectAtomIndex::find_row(formalism::datalog::PredicateView<formalism::FluentTag> applicability_predicate,
                                                     std::span<const ygg::Index<formalism::Object>> binding)
{
    m_lookup.objects.clear();
    m_lookup.objects.insert(m_lookup.objects.end(), binding.begin(), binding.end());
    m_lookup.relation = applicability_predicate.get_index();
    const auto atom = m_repository->find(m_lookup);
    if (!atom)
        return std::nullopt;
    return ygg::uint_t(atom->get_index().row);
}

}
