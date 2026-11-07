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

#ifndef TYR_FORMALISM_WRITE_CONTEXT_HPP_
#define TYR_FORMALISM_WRITE_CONTEXT_HPP_

#include "tyr/formalism/declarations.hpp"

#include <cassert>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>

namespace tyr::formalism
{

/// Where grounding, copying or merging writes: a builder B and a destination repository R. Grounding also reads the
/// objects bound to the parameters (binding()). The objects are the caller's and outlive the context.
template<typename B, typename R>
class WriteContext
{
public:
    WriteContext(B& builder, R& destination) noexcept : m_builder(&builder), m_destination(&destination) {}
    WriteContext(B& builder, R& destination, urd::flat_vector<urd::index<Object>>& binding) noexcept :
        m_builder(&builder),
        m_destination(&destination),
        m_binding(&binding)
    {
    }

    [[nodiscard]] B& builder() const noexcept { return *m_builder; }
    [[nodiscard]] R& destination() const noexcept { return *m_destination; }
    [[nodiscard]] urd::flat_vector<urd::index<Object>>& binding() const noexcept
    {
        assert(m_binding && "WriteContext: grounding needs a binding.");
        return *m_binding;
    }

private:
    B* m_builder;
    R* m_destination;
    urd::flat_vector<urd::index<Object>>* m_binding = nullptr;
};

}

#endif
