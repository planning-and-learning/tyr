/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/query_evaluation.hpp"

#include <variant>

namespace tyr::datalog::database
{
size_t QueryEvaluator::memory_usage() const
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
        result += urd::memory_usage(node.result->relation) + state_usage(node.state);
    return result;
}
}
