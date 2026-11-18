/*
 * Copyright (C) 2026 Dominik Drexler
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "tyr/datalog/lifted/database/semantics/incremental/delta.hpp"

#include <urd/config.hpp>

namespace tyr::datalog::database::incremental::detail
{
void require_initialized(bool initialized)
{
    if (!initialized)
        throw std::logic_error("Incremental evaluation: initialize before use.");
}
}

namespace tyr::datalog::database::incremental::detail
{
void apply_delta(const Delta& delta, ygg::FreeScratchRelation& result)
{
    const auto [added, removed] = delta.change();
    for (const auto& held : removed.get_rows())
        ygg::with_values(held,
                         [&](const auto& values)
                         {
                             [[maybe_unused]] const bool erased = result.erase(values);
                             URD_ASSUME(erased);  // a removed row is in the result
                         });
    for (const auto& held : added.get_rows())
        ygg::with_values(held, [&](const auto& values) { result.insert(values); });
}
}
