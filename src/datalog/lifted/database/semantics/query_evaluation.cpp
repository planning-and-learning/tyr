/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/query_evaluation.hpp"

#include <variant>

namespace tyr::datalog::database::detail
{
bool query_accepts(QueryView<QuerySelectEqualTag> selection, const ygg::RowPool::row_type& row)
{ return row.get(selection.get_plan().lhs_position()) == row.get(selection.get_plan().rhs_position()); }
}

namespace tyr::datalog::database::detail
{
bool query_accepts(QueryView<QuerySelectValueTag> selection, const ygg::RowPool::row_type& row)
{ return row.get(selection.get_plan().position()) == selection.get_plan().value(); }
}

namespace tyr::datalog::database
{
size_t QueryEvaluator::memory_usage() const noexcept
{
    // get_if rather than std::visit, which throws for a valueless variant; states are never valueless.
    const auto usage = []<typename Prepared>(const Prepared* prepared) noexcept -> size_t
    {
        if constexpr (std::same_as<Prepared, std::monostate>)
            return 0;
        else
            return prepared ? prepared->memory_usage() : 0;
    };
    const auto state_usage = [&]<typename... States>(const std::variant<States...>& state) noexcept { return (usage(std::get_if<States>(&state)) + ...); };
    size_t result = m_workspace.memory_usage();
    for (const auto& node : m_nodes)
        result += node.result.memory_usage() + state_usage(node.state);
    return result;
}
}
