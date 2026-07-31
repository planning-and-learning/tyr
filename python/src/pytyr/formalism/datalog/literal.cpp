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

#include "binding_utils.hpp"
#include "bindings.hpp"

#include <tyr/formalism/datalog/literal_data.hpp>
#include <tyr/formalism/datalog/literal_index.hpp>
#include <tyr/formalism/datalog/literal_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<FactKind T>
void bind_literal_data(nb::module_& m, const char* name)
{
    using V = ygg::Data<Literal<T>>;
    auto cls = nb::class_<V>(m, name).def(nb::init<AtomView<T>, bool>(), "atom"_a, "polarity"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_literal_view(nb::module_& m, const char* name)
{
    using V = LiteralView<T>;
    auto cls =
        nb::class_<V>(m, name).def("get_index", &V::get_index).def("get_atom", &V::get_atom, nb::keep_alive<0, 1>()).def("get_polarity", &V::get_polarity);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_literal(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<Literal<StaticTag>>>(m, "StaticLiteralIndex");
    ygg::bind_index<ygg::Index<Literal<FluentTag>>>(m, "FluentLiteralIndex");

    bind_literal_data<StaticTag>(m, "StaticLiteralData");
    bind_literal_data<FluentTag>(m, "FluentLiteralData");

    bind_literal_view<StaticTag>(m, "StaticLiteral");
    bind_literal_view<FluentTag>(m, "FluentLiteral");

    repository.def("get_or_create", &get_or_create_data<Literal<StaticTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<Literal<FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::datalog
