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

#ifndef TYR_FORMALISM_DATALOG_GROUNDER_DECL_HPP_
#define TYR_FORMALISM_DATALOG_GROUNDER_DECL_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/repository.hpp"

namespace tyr::formalism::datalog
{
/// Where grounding, copying or merging writes: the objects are the caller's and outlive the context.
class GrounderContext
{
public:
    GrounderContext(Builder& builder, Repository& destination, ygg::IndexList<Object>& binding) noexcept :
        m_builder(&builder),
        m_destination(&destination),
        m_binding(&binding)
    {
    }

    [[nodiscard]] Builder& builder() const noexcept { return *m_builder; }
    [[nodiscard]] Repository& destination() const noexcept { return *m_destination; }
    [[nodiscard]] ygg::IndexList<Object>& binding() const noexcept { return *m_binding; }

private:
    Builder* m_builder;
    Repository* m_destination;
    ygg::IndexList<Object>* m_binding;
};

}

#endif
