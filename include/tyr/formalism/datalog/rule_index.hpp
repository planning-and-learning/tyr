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

#ifndef TYR_FORMALISM_DATALOG_RULE_INDEX_HPP_
#define TYR_FORMALISM_DATALOG_RULE_INDEX_HPP_

#include <yggdrasil/ids/index_mixins.hpp>
#include <yggdrasil/core/types.hpp>
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

namespace ygg
{
template<tyr::formalism::RelationKind R>
struct Index<tyr::formalism::datalog::Rule<R>> : ygg::IndexMixin<ygg::Index<tyr::formalism::datalog::Rule<R>>>
{
    // Inherit constructors
    using Base = ygg::IndexMixin<ygg::Index<tyr::formalism::datalog::Rule<R>>>;
    using Base::Base;
};

}

#endif
