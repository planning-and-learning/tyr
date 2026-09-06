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
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/atom_index.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/worker_index.hpp"

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <yggdrasil/semantics/containers/dynamic_bitset_hash.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::planning
{

struct RandomDistHashTag
{
};

struct LMCutDistHashTag
{
};

template<typename T>
concept DistHashKind = std::same_as<T, RandomDistHashTag> || std::same_as<T, LMCutDistHashTag>;

template<TaskKind Kind, DistHashKind HashKind>
class DistHash;

template<TaskKind Kind>
class DistHash<Kind, RandomDistHashTag>
{
public:
    explicit DistHash(uint64_t seed = 0) noexcept : m_seed(seed) {}

    void initialize(const StateView<Kind>&) const noexcept {}

    ygg::hash_t hash(const ygg::Builder<State<Kind>>& state) const noexcept
    {
        auto result = static_cast<ygg::hash_t>(m_seed);
        ygg::hash_combine(result, state.template get_atoms<::tyr::formalism::FluentTag>());
        ygg::hash_combine(result, state.get_numeric_variables());
        return ygg::fmix64(result);
    }

    ygg::Index<Worker> owner(const ygg::Builder<State<Kind>>& state, size_t num_workers) const noexcept
    {
        assert(num_workers > 0);
        // The ordinary single-worker search path pays no state-hashing cost.
        return ygg::Index<Worker>(static_cast<ygg::uint_t>(num_workers == 1 ? 0 : hash(state) % num_workers));
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

    ygg::hash_t hash(const ygg::Builder<State<Kind>>& state) const noexcept;

    ygg::Index<Worker> owner(const ygg::Builder<State<Kind>>& state, size_t num_workers) const noexcept
    {
        assert(num_workers > 0);
        return ygg::Index<Worker>(static_cast<ygg::uint_t>(num_workers == 1 ? 0 : hash(state) % num_workers));
    }

private:
    struct Feature
    {
        ygg::Index<::tyr::formalism::planning::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>> atom;
        ygg::Data<::tyr::formalism::planning::FDRFact<::tyr::formalism::FluentTag>> fact;
    };

    DistHash<Kind, RandomDistHashTag> m_fallback;
    uint64_t m_seed;
    std::vector<Feature> m_features;
    bool m_initialized { false };
};

extern template class DistHash<GroundTag, LMCutDistHashTag>;
extern template class DistHash<LiftedTag, LMCutDistHashTag>;

}

#endif
