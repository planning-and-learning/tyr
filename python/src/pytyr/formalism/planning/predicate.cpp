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

namespace tyr::formalism::planning
{

namespace
{

template<FactKind T>
void bind_predicate_data(nb::module_& m, const char* name)
{
    using namespace nb::literals;
    using V = urd::data<Predicate<T>>;
    auto cls = nb::class_<V>(m, name).def(nb::init<const std::string&, ygg::uint_t>(), "name"_a, "arity"_a);
    cls.def(nb::init<urd::flat_string, ygg::uint_t>(), "name"_a, "arity"_a);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}

template<FactKind T>
void bind_predicate_view(nb::module_& m, const char* name)
{
    using V = PredicateView<T>;
    auto cls = nb::class_<V>(m, name).def("index", &V::index).def("get_name", &V::get_name).def("get_arity", &V::get_arity);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}
}  // namespace

void bind_predicate(nb::module_& m, RepositoryBinding& repository)
{
    urd::python::bind_index<urd::index<Predicate<DerivedTag>>>(m, "DerivedPredicateIndex");
    bind_predicate_data<DerivedTag>(m, "DerivedPredicateData");

    bind_predicate_view<StaticTag>(m, "StaticPredicate");
    bind_predicate_view<FluentTag>(m, "FluentPredicate");
    bind_predicate_view<DerivedTag>(m, "DerivedPredicate");

    ygg::bind_insert<Predicate<StaticTag>>(repository);
    ygg::bind_insert<Predicate<FluentTag>>(repository);
    ygg::bind_insert<Predicate<DerivedTag>>(repository);
}

}  // namespace tyr::formalism::planning
