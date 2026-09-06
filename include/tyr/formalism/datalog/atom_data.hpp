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

#ifndef TYR_FORMALISM_DATALOG_ATOM_DATA_HPP_
#define TYR_FORMALISM_DATALOG_ATOM_DATA_HPP_

#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/datalog/atom_index.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/object_index.hpp"
#include "tyr/formalism/predicate_index.hpp"
#include "tyr/formalism/term_data.hpp"

#include <yggdrasil/core/types.hpp>
#include <yggdrasil/core/types_utils.hpp>

namespace ygg
{

template<::tyr::formalism::FactKind F>
struct Data<::tyr::formalism::datalog::Atom<::tyr::LiftedTag, F>>
{
    ygg::Index<::tyr::formalism::datalog::Atom<::tyr::LiftedTag, F>> index;
    ygg::Index<::tyr::formalism::Predicate<F>> predicate;
    ygg::DataList<::tyr::formalism::Term> terms;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::Predicate<F>> predicate_, ygg::DataList<::tyr::formalism::Term> terms_) :
        index(),
        predicate(predicate_),
        terms(std::move(terms_))
    {
    }
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::Predicate<F>>, C> predicate_, const std::vector<::ygg::View<ygg::Data<::tyr::formalism::Term>, C>>& terms_) :
        index(),
        predicate(),
        terms()
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

static_assert(!ygg::uses_trivial_storage_v<::tyr::formalism::datalog::Atom<::tyr::LiftedTag, ::tyr::formalism::StaticTag>>);

template<::tyr::formalism::FactKind F>
struct Data<::tyr::formalism::datalog::Atom<::tyr::GroundTag, F>>
{
    ygg::Index<::tyr::formalism::datalog::Atom<::tyr::GroundTag, F>> index;
    ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::Predicate<F>>> binding;

    Data() = default;
    Data(ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::Predicate<F>>> binding_) : index(), binding(binding_) {}
    template<typename C>
    Data(::ygg::View<ygg::Index<::tyr::formalism::RelationBinding<::tyr::formalism::Predicate<F>>>, C> binding_) : index(), binding()
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

static_assert(ygg::uses_trivial_storage_v<::tyr::formalism::datalog::Atom<::tyr::GroundTag, ::tyr::formalism::StaticTag>>);

}

#endif
