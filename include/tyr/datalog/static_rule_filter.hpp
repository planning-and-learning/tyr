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

#ifndef TYR_DATALOG_STATIC_RULE_FILTER_HPP_
#define TYR_DATALOG_STATIC_RULE_FILTER_HPP_

#include "tyr/formalism/datalog/declarations.hpp"

namespace tyr::datalog
{

::tyr::formalism::datalog::ProgramView<::tyr::GroundTag> remove_statically_inapplicable_rules(::tyr::formalism::datalog::ProgramView<::tyr::GroundTag> program,
                                                                                  ::tyr::formalism::datalog::Repository& repository);

}

#endif
