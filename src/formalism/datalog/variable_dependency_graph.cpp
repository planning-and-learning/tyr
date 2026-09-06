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

#include "tyr/formalism/datalog/variable_dependency_graph.hpp"

#include "tyr/formalism/datalog/expression_arity.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/views.hpp"

#include <algorithm>

namespace tyr::formalism::datalog
{
template<FactKind F, PolarityKind P>
static void insert_literal_dependencies(const std::vector<ParameterIndex>& parameters, details::UnaryDependencies& unary, details::BinaryDependencies& binary)
{
    auto& unary_dep = details::select_literal_dependency<F, P>(unary);
    auto& binary_dep = details::select_literal_dependency<F, P>(binary);

    for (const auto parameter : parameters)
    {
        const auto pi = ygg::uint_t(parameter);
        unary_dep.set(pi);
    }

    for (ygg::uint_t i = 0; i < parameters.size(); ++i)
    {
        const auto pi = ygg::uint_t(parameters[i]);

        for (ygg::uint_t j = i + 1; j < parameters.size(); ++j)
        {
            const auto pj = ygg::uint_t(parameters[j]);

            binary_dep.set(binary.get_index(pi, pj));
            binary_dep.set(binary.get_index(pj, pi));
        }
    }
}

template<FactKind F>
static void insert_literal(LiteralView<::tyr::LiftedTag, F> literal, details::UnaryDependencies& unary, details::BinaryDependencies& binary)
{
    const auto parameters_set = collect_parameters(literal);
    auto parameters = std::vector<ParameterIndex>(parameters_set.begin(), parameters_set.end());
    std::sort(parameters.begin(), parameters.end());

    if (literal.get_polarity())
        insert_literal_dependencies<F, PositiveTag>(parameters, unary, binary);
    else
        insert_literal_dependencies<F, NegativeTag>(parameters, unary, binary);
}

static void insert_numeric_constraint(LiftedBooleanOperatorView numeric_constraint, details::UnaryDependencies& unary, details::BinaryDependencies& binary)
{
    const auto parameters_set = collect_parameters(numeric_constraint);
    auto parameters = std::vector<ParameterIndex>(parameters_set.begin(), parameters_set.end());
    std::sort(parameters.begin(), parameters.end());

    auto& unary_dep = details::select_numeric_dependency(unary);
    auto& binary_dep = details::select_numeric_dependency(binary);

    for (const auto parameter : parameters)
    {
        const auto pi = ygg::uint_t(parameter);
        unary_dep.set(pi);
    }

    for (ygg::uint_t i = 0; i < parameters.size(); ++i)
    {
        const auto pi = ygg::uint_t(parameters[i]);

        for (ygg::uint_t j = i + 1; j < parameters.size(); ++j)
        {
            const auto pj = ygg::uint_t(parameters[j]);

            binary_dep.set(binary.get_index(pi, pj));
            binary_dep.set(binary.get_index(pj, pi));
        }
    }
}

VariableDependencyGraph::VariableDependencyGraph(ConjunctiveConditionView<::tyr::LiftedTag> condition) :
    m_k(condition.get_arity()),
    m_unary_dependencies(m_k),
    m_binary_dependencies(m_k)
{
    for (const auto literal : condition.get_literals<StaticTag>())
        insert_literal(literal, m_unary_dependencies, m_binary_dependencies);

    for (const auto literal : condition.get_literals<FluentTag>())
        insert_literal(literal, m_unary_dependencies, m_binary_dependencies);

    for (const auto numeric_constraint : condition.get_numeric_constraints())
        insert_numeric_constraint(numeric_constraint, m_unary_dependencies, m_binary_dependencies);
}
}
