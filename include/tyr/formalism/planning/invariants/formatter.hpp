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

#ifndef TYR_FORMALISM_PLANNING_INVARIANTS_FORMATTER_HPP_
#define TYR_FORMALISM_PLANNING_INVARIANTS_FORMATTER_HPP_

#include "tyr/formalism/formatter.hpp"
#include "tyr/formalism/planning/invariants/invariant.hpp"
#include "tyr/formalism/planning/mutable/formatter.hpp"

#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <ostream>
#include <sstream>
#include <yggdrasil/formatting/cista_formatters.hpp>
#include <yggdrasil/io/iostream.hpp>

namespace fmt
{

template<>
struct formatter<tyr::formalism::planning::invariant::Invariant, char>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const tyr::formalism::planning::invariant::Invariant& value, FormatContext& ctx) const
    {
        auto os = std::stringstream {};
        os << "Invariant(\n";

        {
            ygg::IndentScope indent { os };

            os << ygg::print_indent;
            fmt::print(os, "num_rigid_variables = {},\n", value.num_rigid_variables);
            os << ygg::print_indent;
            fmt::print(os, "num_counted_variables = {},\n", value.num_counted_variables);
            os << ygg::print_indent;
            fmt::print(os, "atoms = {},\n", value.atoms);
            os << ygg::print_indent;
            fmt::print(os, "predicates = {},\n", value.predicates);
        }

        os << ")";

        return fmt::format_to(ctx.out(), "{}", os.view());
    }
};

}

#endif
