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
#include "tyr/formalism/planning/invariants/invariant.hpp"
#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/unification/match.hpp"
#include "tyr/formalism/unification/structure_traits_impl.hpp"
#include "tyr/formalism/unification/substitution.hpp"
#include "tyr/formalism/unification/term_operations.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>

namespace tyr::formalism::planning::invariant
{

InvariantMatchData::InvariantMatchData(Invariant inv, std::vector<unification::SubstitutionFunction<ygg::Index<Object>>> rigid_variable_bindings) :
    m_inv(std::move(inv)),
    m_rigid_variable_bindings(std::move(rigid_variable_bindings)),
    m_postings()
{
    build_postings();
}

void InvariantMatchData::build_postings()
{
    const auto num_rigid = m_inv.num_rigid_variables;
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

            const auto object = *binding[parameter];

            auto& bitset = m_postings[p][object];
            if (bitset.size() == 0)
                bitset.resize(num_bindings);

            bitset.set(b);
        }
    }
}

namespace
{
bool intersect_matching_bindings(const InvariantMatchData& data, const InvariantSubstitution& sigma, boost::dynamic_bitset<>& candidates)
{
    const auto& inv = data.invariant();
    const auto& postings = data.postings();
    const auto num_bindings = data.rigid_variable_bindings().size();

    candidates.resize(num_bindings);
    candidates.set();

    for (size_t p = 0; p < inv.num_rigid_variables; ++p)
    {
        const auto parameter = ParameterIndex(static_cast<ygg::uint_t>(p));

        if (!sigma.is_bound(parameter))
            continue;

        const auto& value = *sigma[parameter];
        const auto object = unification::get_object(value);

        const auto it = postings[p].find(object);
        if (it == postings[p].end())
        {
            candidates.reset();
            return false;
        }

        candidates &= it->second;
        if (candidates.none())
            return false;
    }

    return !candidates.none();
}
}

Matcher::Matcher(InvariantMatchDataList invariant_match_data) : m_invariant_match_data(std::move(invariant_match_data)) {}

void Matcher::query(const MutableAtom<FluentTag>& atom, QueryResult& result, QueryWorkspace& ws) const
{
    result.matches.clear();

    for (size_t invariant_index = 0; invariant_index < m_invariant_match_data.size(); ++invariant_index)
    {
        const auto& data = m_invariant_match_data[invariant_index];
        const auto& inv = data.invariant();
        const auto num_bindings = data.rigid_variable_bindings().size();

        if (num_bindings == 0)
            continue;

        ws.matches.resize(num_bindings);
        ws.matches.reset();

        for (const auto& pattern : inv.atoms)
        {
            if (pattern.predicate != atom.predicate)
                continue;

            if (pattern.terms.size() != atom.terms.size())
                continue;

            auto sigma = match_invariant_against_ground_atom(inv, pattern, atom);
            if (!sigma.has_value())
                continue;

            if (intersect_matching_bindings(data, *sigma, ws.candidates))
                ws.matches |= ws.candidates;
        }

        for (size_t binding_index = ws.matches.find_first(); binding_index != boost::dynamic_bitset<>::npos;
             binding_index = ws.matches.find_next(binding_index))
        {
            result.matches.push_back({ invariant_index, binding_index });
        }
    }
}

void Matcher::query(AtomView<GroundTag, FluentTag> atom, QueryResult& result, QueryWorkspace& workspace) const
{
    query(MutableAtom<FluentTag>(atom), result, workspace);
}

QueryResult Matcher::query(const MutableAtom<FluentTag>& atom) const
{
    QueryResult result;
    QueryWorkspace workspace;
    query(atom, result, workspace);
    return result;
}

QueryResult Matcher::query(AtomView<GroundTag, FluentTag> atom) const
{
    QueryResult result;
    QueryWorkspace workspace;
    query(atom, result, workspace);
    return result;
}

}
