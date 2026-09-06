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

#ifndef TYR_FORMALISM_PLANNING_MERGE_PLANNING_DECL_HPP_
#define TYR_FORMALISM_PLANNING_MERGE_PLANNING_DECL_HPP_

#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/repository.hpp"

namespace tyr::formalism::planning
{

struct MergePlanningContext
{
    Builder& builder;
    Repository& destination;
};

template<typename T>
struct to_planning_payload
{
    using type = T;  // default: unchanged
};

template<::tyr::TaskKind T>
struct to_planning_payload<ygg::Data<::tyr::formalism::datalog::FunctionExpression<T>>>
{
    using type = ygg::Data<::tyr::formalism::planning::FunctionExpression<T>>;
};

template<typename T>
using to_planning_payload_t = typename to_planning_payload<T>::type;

}

#endif
