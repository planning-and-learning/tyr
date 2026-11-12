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

#ifndef TYR_FORMALISM_DATALOG_RULE_DATA_HPP_
#define TYR_FORMALISM_DATALOG_RULE_DATA_HPP_

#include "tyr/formalism/binary_operator_data.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/numeric_effect_data.hpp"

#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace urd
{
template<>
struct data<::tyr::formalism::datalog::Rule<::tyr::LiftedTag>>
{
    urd::index<::tyr::formalism::datalog::Rule<::tyr::LiftedTag>> index;
    urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables;
    urd::index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>> body;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> heads;
    ygg::float_t cost { 0 };  ///< The edge cost of deriving a head.

    data() = default;
    data(urd::flat_vector<urd::index<::tyr::formalism::Variable>> variables_,
         urd::index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::LiftedTag>> body_,
         urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::LiftedTag, ::tyr::formalism::FluentTag>>> heads_,
         ygg::float_t cost_ = 0) :
        index(),
        variables(std::move(variables_)),
        body(body_),
        heads(std::move(heads_)),
        cost(cost_)
    {
    }
    data(const data& other) = delete;
    data& operator=(const data& other) = delete;
    data(data&& other) = default;
    data& operator=(data&& other) = default;
    ~data() = default;

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::variables, &data::body, &data::heads, &data::cost> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::variables, &data::body, &data::heads, &data::cost> {}; }
};

template<>
struct data<::tyr::formalism::datalog::Rule<::tyr::GroundTag>>
{
    urd::index<::tyr::formalism::datalog::Rule<::tyr::GroundTag>> index;
    ygg::RowIndex<::tyr::formalism::datalog::Rule<::tyr::LiftedTag>> row;
    urd::index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>> body;
    urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> heads;
    ygg::float_t cost { 0 };  ///< The edge cost of deriving a head.

    data() = default;
    data(ygg::RowIndex<::tyr::formalism::datalog::Rule<::tyr::LiftedTag>> row_,
         urd::index<::tyr::formalism::datalog::ConjunctiveCondition<::tyr::GroundTag>> body_,
         urd::flat_vector<urd::index<::tyr::formalism::Atom<::tyr::GroundTag, ::tyr::formalism::FluentTag>>> heads_,
         ygg::float_t cost_ = 0) :
        index(),
        row(row_),
        body(body_),
        heads(std::move(heads_)),
        cost(cost_)
    {
    }

    static constexpr auto cista_members() noexcept { return urd::Members<&data::index, &data::row, &data::body, &data::heads, &data::cost> {}; }
    static constexpr auto identifying_members() noexcept { return urd::Members<&data::row> {}; }
};
}

#endif
