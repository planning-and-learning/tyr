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

#ifndef TYR_FORMALISM_PLANNING_GROUND_FUNCTION_TERM_DATA_HPP_
#define TYR_FORMALISM_PLANNING_GROUND_FUNCTION_TERM_DATA_HPP_

#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/function_index.hpp"
#include "tyr/formalism/object_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/ground_function_term_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<::tyr::formalism::FactKind T>
struct Data<::tyr::formalism::planning::GroundFunctionTerm<T>>
{
    ygg::Index<::tyr::formalism::planning::GroundFunctionTerm<T>> index;
    ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::Function<T>>> binding;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::Function<T>>> binding_) : index(), binding(binding_) {}
    // Python constructor
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::Function<T>>>, C> binding_) : index(), binding()
    {
        set(binding_, binding);
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(binding);
    }

    auto cista_members() const noexcept { return std::tie(index, binding); }
    auto identifying_members() const noexcept { return std::tie(binding); }
};

static_assert(ygg::uses_trivial_storage_v<::tyr::formalism::planning::GroundFunctionTerm<::tyr::formalism::StaticTag>>);
}

#endif
