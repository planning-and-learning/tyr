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

#ifndef TYR_FORMALISM_PLANNING_ATOM_DATA_HPP_
#define TYR_FORMALISM_PLANNING_ATOM_DATA_HPP_

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>
#include "tyr/formalism/planning/atom_index.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/predicate_index.hpp"
#include "tyr/formalism/term_data.hpp"

namespace ygg
{
using namespace ::tyr;

template<::tyr::formalism::FactKind T>
struct Data<::tyr::formalism::planning::Atom<T>>
{
    ygg::Index<::tyr::formalism::planning::Atom<T>> index;
    ygg::Index<::tyr::formalism::Predicate<T>> predicate;
    ygg::DataList<::tyr::formalism::Term> terms;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::Predicate<T>> predicate_, ygg::DataList<::tyr::formalism::Term> terms_) :
        index(),
        predicate(predicate_),
        terms(std::move(terms_))
    {
    }
    // Python constructor
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::Predicate<T>>, C> predicate_, const std::vector<::ygg::View<ygg::Data<::tyr::formalism::Term>, C>>& terms_) : index(), predicate(), terms()
    {
        set(predicate_, predicate);
        set(terms_, terms);
    }
    Data(const Data& other) = delete;
    Data& operator=(const Data& other) = delete;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(predicate);
        ygg::clear(terms);
    }

    auto cista_members() const noexcept { return std::tie(index, predicate, terms); }
    auto identifying_members() const noexcept { return std::tie(predicate, terms); }
};

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::planning::Atom<::tyr::formalism::StaticTag>>);
}

#endif
