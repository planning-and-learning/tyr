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
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"

#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include <urd/elements/flat.hpp>

namespace urd
{

template<::tyr::formalism::FactKind F>
struct data<::tyr::formalism::datalog::Atom<::tyr::LiftedTag, F>>
{
    urd::index<::tyr::formalism::datalog::Atom<::tyr::LiftedTag, F>> index;
    urd::index<::tyr::formalism::Predicate<F>> predicate;
    ::tyr::formalism::TermList terms;

    data() = default;
    data(urd::index<::tyr::formalism::Predicate<F>> predicate_, ::tyr::formalism::TermList terms_) :
        index(),
        predicate(predicate_),
        terms(std::move(terms_))
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::predicate, &data::terms> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::predicate, &data::terms> {}; }
};

template<::tyr::formalism::FactKind F>
struct data<::tyr::formalism::datalog::Atom<::tyr::GroundTag, F>>
{
    urd::index<::tyr::formalism::datalog::Atom<::tyr::GroundTag, F>> index;
    ygg::formalism::RelationBindingIndex<::tyr::formalism::Predicate<F>, ::tyr::formalism::ObjectTag> binding;

    data() = default;
    data(ygg::formalism::RelationBindingIndex<::tyr::formalism::Predicate<F>, ::tyr::formalism::ObjectTag> binding_) : index(), binding(binding_) {}

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::binding> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::binding> {}; }
};

}

#endif
