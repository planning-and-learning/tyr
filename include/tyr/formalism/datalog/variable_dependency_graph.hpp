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

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/variable_dependency_graph_details.hpp"
#include "tyr/formalism/datalog/atom_view.hpp"
#include "tyr/formalism/datalog/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conditional_effect_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/function_term_value_view.hpp"
#include "tyr/formalism/datalog/function_term_view.hpp"
#include "tyr/formalism/datalog/literal_view.hpp"
#include "tyr/formalism/datalog/multi_operator_view.hpp"
#include "tyr/formalism/datalog/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/datalog/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <yggdrasil/core/config.hpp>

namespace tyr::formalism::datalog
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

#endif
