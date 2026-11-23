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

#include "tyr/declarations.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"

#include <atomic>
#include <mutex>
#include <optional>
#include <urd/kinds.hpp>
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <urd/tags.hpp>

namespace tyr::formalism
{

class FDRContext
{
public:
    // Construct without mutexes.
    explicit FDRContext(RepositoryPtr context);

    // Construct with ground mutexes.
    FDRContext(const urd::Store<urd::sequence<AtomViewList<GroundTag, FluentTag>>>& mutexes, RepositoryPtr context);

    // Construct with binary ground mutexes.
    FDRContext(const AtomViewList<GroundTag, FluentTag>& all_atoms, RepositoryPtr context);

    /// Over a frozen parent: finds the parent's facts and starts from its variables; registers what the parent lacks
    /// itself, into `context`, a layer over the parent's repository. Like a repository layer, it does not own its parent:
    /// whoever creates it keeps the parent alive and frozen for its lifetime.
    FDRContext(const FDRContext* parent, RepositoryPtr context);

    FDRContext(const FDRContext&) = delete;
    FDRContext& operator=(const FDRContext&) = delete;
    FDRContext(FDRContext&&) = delete;
    FDRContext& operator=(FDRContext&&) = delete;
    /// Defined in the source, where the builder is complete.
    ~FDRContext();

    FDRFactView<FluentTag> get_fact(AtomView<GroundTag, FluentTag> atom);

    [[nodiscard]] std::optional<FDRFactView<FluentTag>> get_fact(AtomView<GroundTag, FluentTag> atom) const;

    /// Concurrent get_fact calls must have completed before accessing the variable list.
    [[nodiscard]] const FDRVariableViewList<FluentTag>& get_variables() const noexcept;

private:
    struct FactSlot
    {
        urd::data<FDRFact<FluentTag>> fact;
        std::atomic_bool ready { false };
    };

    [[nodiscard]] std::optional<FDRFactView<FluentTag>> find_fact(AtomView<GroundTag, FluentTag> atom) const;
    void ensure_fact_slot(AtomView<GroundTag, FluentTag> atom);
    bool publish_fact(AtomView<GroundTag, FluentTag> atom, urd::data<FDRFact<FluentTag>> fact);

    const FDRContext* m_parent = nullptr;
    RepositoryPtr m_context;
    // Facts are append-only and lock-free to read; only first registration serializes the reusable builder and variable list.
    urd::Store<urd::sequence<FactSlot>, urd::stable_refs, urd::concurrent> m_facts;
    std::mutex m_registration_mutex;
    Builder m_builder;
    FDRVariableViewList<FluentTag> m_variables;
};

}

#endif
