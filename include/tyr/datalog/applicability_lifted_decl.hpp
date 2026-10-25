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

#ifndef TYR_DATALOG_APPLICABILITY_LIFTED_DECL_HPP_
#define TYR_DATALOG_APPLICABILITY_LIFTED_DECL_HPP_

#include "tyr/datalog/declarations.hpp"
#include "tyr/formalism/datalog/declarations.hpp"

namespace tyr::datalog
{
class ApplicabilityContext
{
public:
    ApplicabilityContext(const FactSets& fact_sets, formalism::datalog::GrounderContext& grounder) : m_fact_sets(&fact_sets), m_grounder(&grounder) {}

    [[nodiscard]] const FactSets& fact_sets() const noexcept { return *m_fact_sets; }
    [[nodiscard]] formalism::datalog::GrounderContext& grounder() const noexcept { return *m_grounder; }

private:
    const FactSets* m_fact_sets;
    formalism::datalog::GrounderContext* m_grounder;
};
}

#endif
