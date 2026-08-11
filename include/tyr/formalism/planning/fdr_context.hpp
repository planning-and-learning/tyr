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

#ifndef TYR_FORMALISM_FDR_CONTEXT_HPP_
#define TYR_FORMALISM_FDR_CONTEXT_HPP_

#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"

#include <atomic>
#include <mutex>
#include <optional>
#include <vector>
#include <yggdrasil/containers/segmented_vector.hpp>
#include <yggdrasil/core/types.hpp>

namespace tyr::formalism::planning
{

class FDRContext
{
public:
    // Construct without mutexes.
    explicit FDRContext(RepositoryPtr context);

    // Construct with ground mutexes.
    FDRContext(const std::vector<GroundAtomViewList<FluentTag>>& mutexes, RepositoryPtr context);

    // Construct with binary ground mutexes.
    FDRContext(const GroundAtomViewList<FluentTag>& all_atoms, RepositoryPtr context);

    // Copy the FDRContext after concurrent registrations have completed.
    FDRContext(const FDRContext& other, Builder& builder, RepositoryPtr context);

    FDRFactView<FluentTag> get_fact(GroundAtomView<FluentTag> atom);

    std::optional<FDRFactView<FluentTag>> get_fact(GroundAtomView<FluentTag> atom) const;

    /// Concurrent get_fact calls must have completed before accessing the variable list.
    const FDRVariableViewList<FluentTag>& get_variables() const noexcept;

private:
    struct FactSlot
    {
        ygg::Data<FDRFact<FluentTag>> fact;
        std::atomic_bool ready { false };
    };

    std::optional<FDRFactView<FluentTag>> find_fact(GroundAtomView<FluentTag> atom) const;
    void ensure_fact_slot(GroundAtomView<FluentTag> atom);
    bool publish_fact(GroundAtomView<FluentTag> atom, ygg::Data<FDRFact<FluentTag>> fact);

    RepositoryPtr m_context;
    // Facts are append-only and lock-free to read; only first registration serializes the reusable builder and variable list.
    ygg::SegmentedVector<FactSlot, 32, true> m_facts;
    std::mutex m_registration_mutex;
    Builder m_builder;
    FDRVariableViewList<FluentTag> m_variables;
};

}

#endif
