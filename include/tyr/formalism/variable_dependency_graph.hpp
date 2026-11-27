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

#ifndef TYR_FORMALISM_VARIABLE_DEPENDENCY_GRAPH_HPP_
#define TYR_FORMALISM_VARIABLE_DEPENDENCY_GRAPH_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/variable_dependency_graph_details.hpp"

#include <fmt/base.h>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formatting/without_format_spec.hpp>

namespace tyr::formalism
{

class VariableDependencyGraph
{
public:
    explicit VariableDependencyGraph(ConjunctiveConditionView<LiftedTag> condition);

    [[nodiscard]] const auto& unary() const noexcept { return m_unary_dependencies; }

    [[nodiscard]] const auto& binary() const noexcept { return m_binary_dependencies; }

    /**
     * Getters
     */

    [[nodiscard]] auto k() const noexcept { return m_k; }

private:
    ygg::uint_t m_k;

    details::UnaryDependencies m_unary_dependencies;
    details::BinaryDependencies m_binary_dependencies;
};
}

/// A condition's variable dependency graph: one DOT graph per kind of binary dependency.
template<>
struct fmt::formatter<tyr::formalism::VariableDependencyGraph> : ygg::without_format_spec
{
    fmt::format_context::iterator format(const tyr::formalism::VariableDependencyGraph& value, fmt::format_context& context) const;
};

#endif
