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

#ifndef TYR_FORMALISM_DATALOG_DECLARATIONS_HPP_
#define TYR_FORMALISM_DATALOG_DECLARATIONS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <boost/mp11.hpp>
#include <cista/containers/variant.h>
#include <cista/containers/vector.h>
#include <concepts>
#include <memory>
#include <string_view>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <variant>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/builder.hpp>

namespace tyr::formalism
{

/**
 * Declaration-only view aliases.
 *
 * Include repository.hpp before constructing, storing, or accessing views.
 */

using RuleRowView = ygg::RowView<Rule<LiftedTag>, Repository>;

using RuleRowIndex = ygg::RowIndex<Rule<LiftedTag>>;

/// The body of a rule of kind T: a lifted rule's shared conjunctive condition, a ground rule's RuleBody.
template<TaskKind T>
using RuleBodyView = std::
    conditional_t<std::same_as<T, GroundTag>, urd::index_view<RuleBody<GroundTag>, Repository>, urd::index_view<ConjunctiveCondition<LiftedTag>, Repository>>;
template<TaskKind T>
using ProgramView = urd::index_view<Program<T>, Repository>;
template<TaskKind T>
using RuleView = urd::index_view<Rule<T>, Repository>;
template<TaskKind T>
using RuleViewList = urd::Store<urd::sequence<RuleView<T>>>;

}

#endif
