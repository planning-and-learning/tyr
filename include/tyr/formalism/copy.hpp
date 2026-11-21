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

#ifndef TYR_FORMALISM_COPY_HPP_
#define TYR_FORMALISM_COPY_HPP_

#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/write_context.hpp"

#include <cassert>
#include <urd/elements/children.hpp>
#include <urd/elements/concepts.hpp>
#include <utility>
#include <variant>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/interning.hpp>
#include <yggdrasil/formalism/row_view.hpp>

/// Copying records of one repository S into another R (datalog or planning, the same or the other: the records both
/// store have one type; include R's namespace's interning.hpp, which canonicalizes on insert). A copy is found by ADL on
/// its WriteContext: call it unqualified or as formalism::copy.
namespace tyr::formalism
{

/// A binding: its relation and its objects copied.
template<typename Rel, typename S, typename B, typename R>
std::pair<ygg::RowView<Rel, R>, bool> copy(ygg::RowView<Rel, S> element, WriteContext<B, R>& context)
{
    auto binding = ygg::formalism::checkout<ygg::Row<Rel>>(context.builder());

    urd::store(binding->relation, copy(element.get_relation(), context).first);
    for (const auto object : element.get_objects())
        binding->values.push_back(copy(object, context).first.index().value);

    return context.destination().insert(*binding, context.builder());
}

/// A binding's row stored in `to`, whose layers share the frozen ancestor that holds the binding's relation and objects
/// at the same indices (a program's layers and its task's): the relation and values are taken as they are, nothing else
/// is interned. Its key, and whether it was stored in `to`'s own layer.
template<typename Rel, typename S, typename R>
std::pair<ygg::RowIndex<Rel>, bool> copy_row(ygg::RowView<Rel, S> row, R& to)
{
    const auto relation = row.get_relation().index();
    assert(to.view(relation).get_name() == row.get_relation().get_name());
    const auto [id, inserted] = to.insert_row(relation, row.get_values());
    return { ygg::RowIndex<Rel> { relation, id }, inserted };
}

/// A record: its members copied, each child (every child of its builder, urd::build_from) and its
/// binding (a ground record's) copied first.
template<typename T, typename S, typename B, typename R>
std::pair<urd::index_view<T, R>, bool> copy(urd::index_view<T, S> element, WriteContext<B, R>& context)
{
    auto record = ygg::formalism::checkout<T>(context.builder());

    urd::build_from(element, *record, [&](auto child) { return copy(child, context).first; });
    if constexpr (requires { record->row; })
        urd::store(record->row, copy(element.context().view(record->row), context).first);

    return context.destination().insert(*record, context.builder());
}

/// A variant readout (a term, a function expression) copied alternative by alternative; a value it holds (a
/// parameter, a number) as it is.
template<typename... Vs, typename B, typename R>
std::variant<Vs...> copy(const std::variant<Vs...>& element, WriteContext<B, R>& context)
{
    return std::visit(
        [&]<typename Alternative>(const Alternative& arg) -> std::variant<Vs...>
        {
            if constexpr (requires { copy(arg, context); })
                return copy(arg, context).first;
            else
                return arg;
        },
        element);
}

}

#endif
