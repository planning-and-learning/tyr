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

#ifndef TYR_FORMALISM_DATALOG_VARIABLE_DEPENDENCY_GRAPH_HPP_
#define TYR_FORMALISM_DATALOG_VARIABLE_DEPENDENCY_GRAPH_HPP_

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/containers/variant.hpp>
#include <yggdrasil/containers/vector.hpp>
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/variable_dependency_graph_details.hpp"
#include "tyr/formalism/datalog/views.hpp"

namespace tyr::formalism::datalog
{

class VariableDependencyGraph
{
public:
    explicit VariableDependencyGraph(ConjunctiveConditionView condition);

    const auto& unary() const noexcept { return m_unary_dependencies; }
    const auto& binary() const noexcept { return m_binary_dependencies; }

    /**
     * Getters
     */

    auto k() const noexcept { return m_k; }

private:
    ygg::uint_t m_k;

    details::UnaryDependencies m_unary_dependencies;
    details::BinaryDependencies m_binary_dependencies;
};
}

#endif
