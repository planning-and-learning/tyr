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
                                 const datalog::TaggedFactSets<formalism::FluentTag>& facts,
                                 const formalism::datalog::Repository& repository) :
    m_effect_to_predicate(&effect_to_predicate),
    m_facts(&facts),
    m_repository(&repository)
{
    for (const auto& [predicate, action] : predicate_to_action)
        m_applicability_predicates.emplace(action.index(), predicate);
}

void EffectAtomIndex::clear() noexcept
{
    for (auto& [_, groups] : m_groups)
        groups.current = false;
}

std::optional<std::pair<EffectAtomIndex::Atoms::const_iterator, EffectAtomIndex::Atoms::const_iterator>> EffectAtomIndex::get_atoms(formalism::planning::ActionView<LiftedTag> action,
                                                                                      formalism::planning::ConditionalEffectView<LiftedTag> effect,
                                                                                      std::span<const urd::index<formalism::Object>> binding)
{
    const auto effect_predicate = m_effect_to_predicate->find(std::pair(action, effect));
    if (effect_predicate == m_effect_to_predicate->end())
        return std::nullopt;

    const auto applicability_predicate = m_applicability_predicates.at(action.index());
    auto& groups = m_groups[effect_predicate->second];
    if (!groups.current)
        rebuild(groups, effect_predicate->second, applicability_predicate, action.get_arity());

    const auto row = find_row(applicability_predicate, binding);
    if (!row)
        return std::pair(groups.atoms.end(), groups.atoms.end());
    return groups.atoms.equal_range(*row);
}

void EffectAtomIndex::rebuild(Groups& groups,
                              formalism::datalog::PredicateView<formalism::FluentTag> effect_predicate,
                              formalism::datalog::PredicateView<formalism::FluentTag> applicability_predicate,
                              size_t action_arity)
{
    const auto atoms = datalog::rows(*m_facts, effect_predicate.index().value);

    // The applicability row of each atom's action binding, which holds since the effect rule's body contains the precondition.
    groups.atoms.clear();
    for (const auto atom : atoms)
    {
        const auto objects = atom.template get<urd::index<formalism::Object>>();
        m_prefix.clear();
        for (size_t i = 0; i < action_arity; ++i)
            m_prefix.push_back(objects[i]);
        const auto row = find_row(applicability_predicate, std::span<const urd::index<formalism::Object>>(m_prefix.data(), m_prefix.size()));
        assert(row && "An effect atom's action binding is applicable.");
        groups.atoms.emplace(*row, atom.data());
    }

    groups.current = true;
}

std::optional<ygg::uint_t> EffectAtomIndex::find_row(formalism::datalog::PredicateView<formalism::FluentTag> applicability_predicate,
                                                     std::span<const urd::index<formalism::Object>> binding)
{
    m_lookup.values.clear();
    for (const auto object : binding)
        m_lookup.values.push_back(object.value);
    m_lookup.relation = applicability_predicate.index();
    const auto atom = m_repository->find(m_lookup);
    if (!atom)
        return std::nullopt;
    return static_cast<ygg::uint_t>(atom->data().row.value);
}

}
