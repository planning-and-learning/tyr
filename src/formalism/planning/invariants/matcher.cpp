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

#include "tyr/formalism/planning/invariants/matcher.hpp"

#include "matching.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/unification/match.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"
#include "tyr/formalism/unification/substitution.hpp"
#include "tyr/formalism/unification/term_operations.hpp"

#include <cassert>
#include <cstddef>
#include <ranges>
#include <urd/config.hpp>
#include <urd/elements/clone.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::formalism::invariant
{

namespace
{
/// A ground atom as a lifted atom's record: its objects as its terms, so that it is matched against the patterns.
urd::data<Atom<LiftedTag, FluentTag>> lifted_record(AtomView<GroundTag, FluentTag> atom)
{
    auto result = urd::data<Atom<LiftedTag, FluentTag>>();
    result.predicate = atom.get_predicate().index();
    for (const auto object : atom.get_row().get_objects())
        result.terms.push_back(Term(object.index()));
    return result;
}
}

InvariantMatchData::InvariantMatchData(InvariantView invariant,
                                       urd::Store<urd::sequence<unification::SubstitutionFunction<urd::index<Object>>>> rigid_variable_bindings) :
    m_invariant(invariant),
    m_rigid_variable_bindings(std::move(rigid_variable_bindings)),
    m_postings()
{ build_postings(); }

void InvariantMatchData::build_postings()
{
    const std::size_t num_rigid = m_invariant.get_num_rigid_variables();
    const auto num_bindings = m_rigid_variable_bindings.size();

    m_postings.clear();
    m_postings.resize(num_rigid);

    for (size_t b = 0; b < num_bindings; ++b)
    {
        const auto& binding = m_rigid_variable_bindings[b];

        for (size_t p = 0; p < num_rigid; ++p)
        {
            const auto parameter = ParameterIndex(static_cast<ygg::uint_t>(p));

            // Optional but recommended: validate domain consistency.
            assert(binding.contains_parameter(parameter));

            if (!binding.is_bound(parameter))
                continue;

            const auto& bound = binding[parameter];
            URD_ASSUME(bound.has_value());
            const auto object = *bound;

            m_postings[p][object].insert(b);
        }
    }
}

namespace
{
/// The bindings that bind each rigid parameter sigma binds to the same object, into candidates; whether there is one.
bool intersect_matching_bindings(const InvariantMatchData& data, const InvariantSubstitution& sigma, BindingSet& candidates)
{
    const auto& postings = data.postings();

    auto first = true;
    for (size_t p = 0; p < data.invariant().get_num_rigid_variables(); ++p)
    {
        const auto parameter = ParameterIndex(static_cast<ygg::uint_t>(p));

        if (!sigma.is_bound(parameter))
            continue;

        const auto& bound = sigma[parameter];
        URD_ASSUME(bound.has_value());
        const auto& value = *bound;
        const auto object = unification::get_object(value);

        const auto it = postings[p].find(object);
        if (it == postings[p].end())
            return false;

        if (first)
            candidates = it->second;
        else
            candidates &= it->second;
        first = false;
        if (candidates.empty())
            return false;
    }

    if (first)
    {
        // sigma binds no rigid parameter: every binding.
        const auto all = std::views::iota(size_t { 0 }, data.rigid_variable_bindings().size());
        candidates.clear();
        candidates.insert(all.begin(), all.end());
    }
    return !candidates.empty();
}
}

Matcher::Matcher(InvariantMatchDataList invariant_match_data) : m_invariant_match_data(std::move(invariant_match_data)) {}

void Matcher::query(AtomView<GroundTag, FluentTag> ground_atom, QueryResult& result, QueryWorkspace& workspace) const
{
    result.matches.clear();
    const auto atom = lifted_record(ground_atom);

    for (size_t invariant_index = 0; invariant_index < m_invariant_match_data.size(); ++invariant_index)
    {
        const auto& data = m_invariant_match_data[invariant_index];
        const auto num_bindings = data.rigid_variable_bindings().size();

        if (num_bindings == 0)
            continue;

        workspace.matches.clear();

        // Each atom of the invariant is a pattern, read as its record.
        for (const auto part : data.invariant().get_atoms())
        {
            const auto& pattern = part.context()[part.index()];
            if (pattern.predicate != atom.predicate)
                continue;

            if (pattern.terms.size() != atom.terms.size())
                continue;

            auto sigma =
                match_invariant_against_ground_atom(data.invariant().get_num_rigid_variables(), data.invariant().get_num_counted_variables(), pattern, atom);
            if (!sigma.has_value())
                continue;

            if (intersect_matching_bindings(data, *sigma, workspace.candidates))
                workspace.matches |= workspace.candidates;
        }

        for (const auto binding_index : workspace.matches)
            result.matches.push_back({ invariant_index, binding_index });
    }
}

QueryResult Matcher::query(AtomView<GroundTag, FluentTag> atom) const
{
    QueryResult result;
    QueryWorkspace workspace;
    query(atom, result, workspace);
    return result;
}

}
