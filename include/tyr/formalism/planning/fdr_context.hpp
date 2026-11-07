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

#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_data.hpp>
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/planning/action_data.hpp"
#include "tyr/formalism/planning/atom_data.hpp"
#include "tyr/formalism/planning/axiom_data.hpp"
#include "tyr/formalism/planning/binary_operator_data.hpp"
#include "tyr/formalism/planning/conditional_effect_data.hpp"
#include "tyr/formalism/planning/conjunctive_condition_data.hpp"
#include "tyr/formalism/planning/conjunctive_effect_data.hpp"
#include "tyr/formalism/planning/domain_data.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_variable_data.hpp"
#include "tyr/formalism/planning/function_term_data.hpp"
#include "tyr/formalism/planning/function_term_value_data.hpp"
#include "tyr/formalism/planning/invariant_data.hpp"
#include "tyr/formalism/planning/literal_data.hpp"
#include "tyr/formalism/planning/metric_data.hpp"
#include "tyr/formalism/planning/multi_operator_data.hpp"
#include "tyr/formalism/planning/numeric_effect_data.hpp"
#include "tyr/formalism/planning/task_data.hpp"
#include "tyr/formalism/planning/unary_operator_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"

#include <atomic>
#include <cstddef>
#include <mutex>
#include <optional>
#include <urd/repr/sequences/segmented.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <vector>
#include <urd/elements/flat.hpp>

namespace tyr::formalism::planning
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

    // Copy the FDRContext after concurrent registrations have completed.
    FDRContext(const FDRContext& other, Builder& builder, RepositoryPtr context);

    FDRFactView<FluentTag> get_fact(AtomView<GroundTag, FluentTag> atom);

    std::optional<FDRFactView<FluentTag>> get_fact(AtomView<GroundTag, FluentTag> atom) const;

    /// Concurrent get_fact calls must have completed before accessing the variable list.
    const FDRVariableViewList<FluentTag>& get_variables() const noexcept;

private:
    struct FactSlot
    {
        urd::data<FDRFact<FluentTag>> fact;
        std::atomic_bool ready { false };
    };

    std::optional<FDRFactView<FluentTag>> find_fact(AtomView<GroundTag, FluentTag> atom) const;
    void ensure_fact_slot(AtomView<GroundTag, FluentTag> atom);
    bool publish_fact(AtomView<GroundTag, FluentTag> atom, urd::data<FDRFact<FluentTag>> fact);

    RepositoryPtr m_context;
    // Facts are append-only and lock-free to read; only first registration serializes the reusable builder and variable list.
    urd::Store<urd::sequence<FactSlot>, urd::stable_refs, urd::concurrent> m_facts;
    std::mutex m_registration_mutex;
    Builder m_builder;
    FDRVariableViewList<FluentTag> m_variables;
};

}

#endif
