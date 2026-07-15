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

#ifndef TYR_FORMALISM_PLANNING_INVARIANTS_MATCHER_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANTS_MATCHER_HPP_

#include "tyr/formalism/planning/invariants/invariant.hpp"
#include "tyr/formalism/planning/mutable/atom.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/unification/match.hpp"
#include "tyr/formalism/unification/substitution.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>

namespace tyr::formalism::planning::invariant
{
struct InvariantMatchData
{
public:
    InvariantMatchData(Invariant inv, std::vector<unification::SubstitutionFunction<ygg::Index<Object>>> rigid_variable_bindings);

    const auto& invariant() const noexcept { return m_inv; }
    const auto& rigid_variable_bindings() const noexcept { return m_rigid_variable_bindings; }
    const auto& postings() const noexcept { return m_postings; }

private:
    void build_postings();

private:
    Invariant m_inv;
    std::vector<unification::SubstitutionFunction<ygg::Index<Object>>> m_rigid_variable_bindings;

    /// postings[p][o][b] is true iff the b-th binding binds object o to rigid parameter p.
    std::vector<ygg::UnorderedMap<ygg::Index<Object>, boost::dynamic_bitset<>>> m_postings;
};

using InvariantMatchDataList = std::vector<InvariantMatchData>;

struct QueryResult
{
    struct Match
    {
        size_t invariant_index;
        size_t binding_index;
    };

    std::vector<Match> matches;
};

struct QueryWorkspace
{
    boost::dynamic_bitset<> candidates;
    boost::dynamic_bitset<> matches;
};

class Matcher
{
public:
    Matcher(InvariantMatchDataList invariant_match_data);

    void query(const MutableAtom<FluentTag>& atom, QueryResult& result, QueryWorkspace& ws) const;

    void query(GroundAtomView<FluentTag> atom, QueryResult& result, QueryWorkspace& workspace) const;

    QueryResult query(const MutableAtom<FluentTag>& atom) const;
    QueryResult query(GroundAtomView<FluentTag> atom) const;

private:
    InvariantMatchDataList m_invariant_match_data;
};

}

#endif
