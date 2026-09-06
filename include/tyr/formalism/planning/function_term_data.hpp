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

#ifndef TYR_FORMALISM_PLANNING_FUNCTION_TERM_DATA_HPP_
#define TYR_FORMALISM_PLANNING_FUNCTION_TERM_DATA_HPP_

#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/function_index.hpp"
#include "tyr/formalism/object_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/function_term_index.hpp"
#include "tyr/formalism/term_data.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<::tyr::formalism::FactKind F>
struct Data<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, F>>
{
    ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, F>> index;
    ygg::Index<::tyr::formalism::Function<F>> function;
    ygg::DataList<::tyr::formalism::Term> terms;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::Function<F>> function_, ygg::DataList<::tyr::formalism::Term> terms_) :
        index(),
        function(function_),
        terms(std::move(terms_))
    {
    }
    // Python constructor
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::Function<F>>, C> function_, const std::vector<::ygg::View<ygg::Data<::tyr::formalism::Term>, C>>& terms_) :
        index(),
        function(),
        terms()
    {
        set(function_, function);
        set(terms_, terms);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(function);
        ygg::clear(terms);
    }

    auto cista_members() const noexcept { return std::tie(index, function, terms); }
    auto identifying_members() const noexcept { return std::tie(function, terms); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::FunctionTerm<::tyr::LiftedTag, ::tyr::formalism::StaticTag>>);

template<::tyr::formalism::FactKind F>
struct Data<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, F>>
{
    ygg::Index<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, F>> index;
    ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::Function<F>>> binding;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::Function<F>>> binding_) : index(), binding(binding_) {}
    // Python constructor
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::Function<F>>>, C> binding_) : index(), binding()
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

static_assert(ygg::uses_trivial_storage_v<::tyr::formalism::planning::FunctionTerm<::tyr::GroundTag, ::tyr::formalism::StaticTag>>);
}

#endif
