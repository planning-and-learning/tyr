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

#ifndef TYR_PLANNING_APPLICABILITY_DECL_HPP_
#define TYR_PLANNING_APPLICABILITY_DECL_HPP_

#include "tyr/planning/declarations.hpp"

#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// What applicability reads of a state: its task, its storages (its fluent facts, derived atoms and numeric values) and
/// an auxiliary value. The state must outlive the context.
template<TaskKind Kind>
class StateContext
{
public:
    /// From a state's view, registered or being built: its storages, through its getters.
    template<typename V>
        requires requires(const V& state) { state.get_fact_storage(); }
    StateContext(const Task<Kind>& task, const V& state, ygg::float_t auxiliary_value) :
        StateContext(task, state.get_fact_storage(), state.get_atom_storage(), state.get_numeric_storage(), auxiliary_value)
    {
    }
    /// From a state being built (its builder: a data<State>), its storages.
    StateContext(const Task<Kind>& task, const urd::data<State<Kind>>& state, ygg::float_t auxiliary_value) :
        StateContext(task, state.fact_storage, state.atom_storage, state.numeric_storage, auxiliary_value)
    {
    }

    [[nodiscard]] const Task<Kind>& task() const noexcept { return *m_task; }
    [[nodiscard]] const FactUnpackedStorage<Kind>& fact_storage() const noexcept { return *m_fact_storage; }
    [[nodiscard]] const AtomUnpackedStorage<Kind>& atom_storage() const noexcept { return *m_atom_storage; }
    [[nodiscard]] const NumericUnpackedStorage<Kind>& numeric_storage() const noexcept { return *m_numeric_storage; }
    [[nodiscard]] ygg::float_t auxiliary_value() const noexcept { return m_auxiliary_value; }
    [[nodiscard]] ygg::float_t& auxiliary_value() noexcept { return m_auxiliary_value; }

private:
    StateContext(const Task<Kind>& task,
                 const FactUnpackedStorage<Kind>& fact_storage,
                 const AtomUnpackedStorage<Kind>& atom_storage,
                 const NumericUnpackedStorage<Kind>& numeric_storage,
                 ygg::float_t auxiliary_value) :
        m_task(&task),
        m_fact_storage(&fact_storage),
        m_atom_storage(&atom_storage),
        m_numeric_storage(&numeric_storage),
        m_auxiliary_value(auxiliary_value)
    {
    }

    const Task<Kind>* m_task;
    const FactUnpackedStorage<Kind>* m_fact_storage;
    const AtomUnpackedStorage<Kind>* m_atom_storage;
    const NumericUnpackedStorage<Kind>* m_numeric_storage;
    ygg::float_t m_auxiliary_value;
};

}

#endif
