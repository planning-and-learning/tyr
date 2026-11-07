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
#include "pytyr/module.hpp"
#include "bindings.hpp"
#include "tyr/formalism/declarations.hpp"

#include <tyr/formalism/predicate_data.hpp>
#include <tyr/formalism/predicate_view.hpp>

namespace tyr::formalism
{

namespace
{
template<FactKind T>
void bind_predicate_data(nb::module_& m, const char* name)
{
    using namespace nb::literals;
    using V = urd::data<Predicate<T>>;
    auto cls = nb::class_<V>(m, name).def(nb::init<const std::string&, ygg::uint_t>(), "name"_a, "arity"_a);
    cls.def(nb::init<::cista::offset::string, ygg::uint_t>(), "name"_a, "arity"_a);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}
}  // namespace

void bind_predicate(nb::module_& m)
{
    urd::python::bind_index<urd::index<Predicate<StaticTag>>>(m, "StaticPredicateIndex");
    urd::python::bind_index<urd::index<Predicate<FluentTag>>>(m, "FluentPredicateIndex");

    bind_predicate_data<StaticTag>(m, "StaticPredicateData");
    bind_predicate_data<FluentTag>(m, "FluentPredicateData");
}

}  // namespace tyr::formalism
