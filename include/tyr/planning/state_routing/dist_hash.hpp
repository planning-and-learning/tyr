/*
 * Copyright (C) 2026 Dominik Drexler
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

#ifndef TYR_PLANNING_STATE_ROUTING_DIST_HASH_HPP_
#define TYR_PLANNING_STATE_ROUTING_DIST_HASH_HPP_

#include "tyr/formalism/declarations.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_routing/dist_hash_concepts.hpp"

#include <boost/hash2/xxhash.hpp>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <urd/elements/hash.hpp>
#include <vector>
#include <urd/elements/hash_dynamic_bitset.hpp>

namespace tyr::planning
{

template<TaskKind Kind, DistHashKind HashKind>
class DistHash;

template<TaskKind Kind>
class DistHash<Kind, RandomDistHashTag>
{
public:
    explicit DistHash(uint64_t seed = 0) noexcept : m_seed(seed) {}

    void initialize(const StateView<Kind>&) const noexcept {}

    [[nodiscard]] ygg::hash_t hash(const urd::data<State<Kind>>& state) const noexcept
    {
        auto result = static_cast<ygg::hash_t>(m_seed);
        urd::hash_combine(result, state.template get_atom_storage<formalism::FluentTag>());
        urd::hash_combine(result, state.get_numeric_variables());
        // Final avalanche over the accumulated hash, using XXH64's 64-bit seed and result.
        return boost::hash2::xxhash_64(result).result();
    }

    [[nodiscard]] urd::index<Worker> owner(const urd::data<State<Kind>>& state, size_t num_workers) const noexcept
    {
        assert(num_workers > 0);
        // The ordinary single-worker search path pays no state-hashing cost.
        return urd::index<Worker>(static_cast<ygg::uint_t>(num_workers == 1 ? 0 : hash(state) % num_workers));
    }

private:
    uint64_t m_seed;
};

template<TaskKind Kind>
class DistHash<Kind, LMCutDistHashTag>
{
public:
    explicit DistHash(uint64_t seed = 0) noexcept : m_fallback(seed), m_seed(seed) {}

    /// Select the initial state's unit-cost LM-cut frontier and positive task goals once before routing.
    void initialize(const StateView<Kind>& start_state);

    [[nodiscard]] ygg::hash_t hash(const urd::data<State<Kind>>& state) const noexcept;

    [[nodiscard]] urd::index<Worker> owner(const urd::data<State<Kind>>& state, size_t num_workers) const noexcept
    {
        assert(num_workers > 0);
        return urd::index<Worker>(static_cast<ygg::uint_t>(num_workers == 1 ? 0 : hash(state) % num_workers));
    }

private:
    struct Feature
    {
        urd::index<formalism::planning::Atom<GroundTag, formalism::FluentTag>> atom;
        urd::data<formalism::planning::FDRFact<formalism::FluentTag>> fact;
    };

    DistHash<Kind, RandomDistHashTag> m_fallback;
    uint64_t m_seed;
    urd::Store<urd::sequence<Feature>> m_features;
    bool m_initialized { false };
};

extern template class DistHash<GroundTag, LMCutDistHashTag>;
extern template class DistHash<LiftedTag, LMCutDistHashTag>;

}

#endif
