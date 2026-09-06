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

#ifndef TYR_FORMALISM_PLANNING_NUMERIC_EFFECT_INDEX_HPP_
#define TYR_FORMALISM_PLANNING_NUMERIC_EFFECT_INDEX_HPP_

#include "tyr/formalism/planning/declarations.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/ids/index_mixins.hpp>

namespace ygg
{
template<::tyr::TaskKind T, tyr::formalism::FactKind F>
struct Index<tyr::formalism::planning::NumericEffect<T, F>> : ygg::IndexMixin<ygg::Index<tyr::formalism::planning::NumericEffect<T, F>>>
{
    static_assert(std::same_as<F, tyr::formalism::FluentTag> || std::same_as<F, tyr::formalism::AuxiliaryTag>, "Unsupported NumericEffect<F> specialization.");

    // Inherit constructors
    using Base = ygg::IndexMixin<ygg::Index<tyr::formalism::planning::NumericEffect<T, F>>>;
    using Base::Base;
};

}

#endif
