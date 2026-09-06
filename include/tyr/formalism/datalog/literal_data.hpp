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

#ifndef TYR_FORMALISM_DATALOG_LITERAL_DATA_HPP_
#define TYR_FORMALISM_DATALOG_LITERAL_DATA_HPP_

#include "tyr/formalism/datalog/atom_index.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/literal_index.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<::tyr::TaskKind T, ::tyr::formalism::FactKind F>
struct Data<::tyr::formalism::datalog::Literal<T, F>>
{
    ygg::Index<::tyr::formalism::datalog::Literal<T, F>> index;
    ygg::Index<::tyr::formalism::datalog::Atom<T, F>> atom;
    bool polarity;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::datalog::Atom<T, F>> atom_, bool polarity_) : index(), atom(atom_), polarity(polarity_) {}
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::datalog::Atom<T, F>>, C> atom_, bool polarity_) : index(), atom(), polarity(polarity_)
    {
        set(atom_, atom);
    }
    Data(const Data& other) = default;
    Data& operator=(const Data& other) = default;
    Data(Data&& other) = default;
    Data& operator=(Data&& other) = default;

    void clear() noexcept
    {
        ygg::clear(index);
        ygg::clear(atom);
        ygg::clear(polarity);
    }

    auto cista_members() const noexcept { return std::tie(index, atom, polarity); }
    auto identifying_members() const noexcept { return std::tie(atom, polarity); }
};

static_assert(ygg::uses_trivial_storage_v<::tyr::formalism::datalog::Literal<::tyr::LiftedTag, ::tyr::formalism::StaticTag>>);
static_assert(ygg::uses_trivial_storage_v<::tyr::formalism::datalog::Literal<::tyr::GroundTag, ::tyr::formalism::StaticTag>>);

}

#endif
