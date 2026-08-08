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

#ifndef TYR_ANALYSIS_VARIABLE_DOMAIN_HPP_
#define TYR_ANALYSIS_VARIABLE_DOMAIN_HPP_

#include "tyr/formalism/object_index.hpp"

#include <vector>

namespace tyr::analysis
{

struct VariableDomain
{
    std::vector<ygg::Index<::tyr::formalism::Object>> objects;
};

using VariableDomainList = std::vector<VariableDomain>;

}

#endif
