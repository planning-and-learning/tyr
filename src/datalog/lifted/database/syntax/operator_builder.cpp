/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/syntax/operator_builder.hpp"

namespace tyr::datalog::database
{
QueryView<> OperatorBuilder::generic_join(std::span<const QueryView<>> atoms, std::span<const urd::index<ygg::Column>> order)
{
    return make<QueryGenericJoinTag>(
        [&](auto& data)
        {
            for (const auto atom : atoms)
                data.inputs.push_back(atom.index());
            data.variable_order.set(order.begin(), order.end());
            data.output_order.set(order.begin(), order.end());
        });
}
}
