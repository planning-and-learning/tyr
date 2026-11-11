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
#include "pytyr/module.hpp"

#include <tyr/formalism/declarations.hpp>
#include <yggdrasil/formalism/row_data.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::formalism
{

namespace
{
/// A row's record from its relation and objects (each object as its canonical Value).
template<typename V, typename R, typename Objects>
void init_row_data(V* self, R relation, const Objects& objects)
{
    new (self) V();
    self->relation = relation.index();
    for (const auto object : objects)
        self->values.push_back(object.index().value);
}

template<typename Relation>
void bind_row_data(nb::module_& m, const char* name)
{
    using namespace nb::literals;
    using V = urd::data<ygg::Row<Relation>>;
    auto cls = nb::class_<V>(m, name)
                   .def("__init__", &init_row_data<V, urd::index_view<Relation, planning::Repository>, planning::ObjectViewList>, "relation"_a, "objects"_a)
                   .def("__init__", &init_row_data<V, urd::index_view<Relation, datalog::Repository>, datalog::ObjectViewList>, "relation"_a, "objects"_a);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}
}  // namespace

void bind_row(nb::module_& m)
{
    {
        using V = ygg::RowIndex<Predicate<StaticTag>>;
        auto cls = nb::class_<V>(m, "StaticPredicateRowIndex")
                       .def_ro("relation_index", &V::relation)
                       .def_prop_ro("row_index", [](const V& self) { return self.row.value; });
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = ygg::RowIndex<Predicate<FluentTag>>;
        auto cls = nb::class_<V>(m, "FluentPredicateRowIndex")
                       .def_ro("relation_index", &V::relation)
                       .def_prop_ro("row_index", [](const V& self) { return self.row.value; });
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = ygg::RowIndex<Function<StaticTag>>;
        auto cls = nb::class_<V>(m, "StaticFunctionRowIndex")
                       .def_ro("relation_index", &V::relation)
                       .def_prop_ro("row_index", [](const V& self) { return self.row.value; });
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = ygg::RowIndex<Function<FluentTag>>;
        auto cls = nb::class_<V>(m, "FluentFunctionRowIndex")
                       .def_ro("relation_index", &V::relation)
                       .def_prop_ro("row_index", [](const V& self) { return self.row.value; });
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = ygg::RowIndex<Function<AuxiliaryTag>>;
        auto cls = nb::class_<V>(m, "AuxiliaryFunctionRowIndex")
                       .def_ro("relation_index", &V::relation)
                       .def_prop_ro("row_index", [](const V& self) { return self.row.value; });
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    bind_row_data<Predicate<StaticTag>>(m, "StaticPredicateRowData");
    bind_row_data<Predicate<FluentTag>>(m, "FluentPredicateRowData");
    bind_row_data<Function<StaticTag>>(m, "StaticFunctionRowData");
    bind_row_data<Function<FluentTag>>(m, "FluentFunctionRowData");
    bind_row_data<Function<AuxiliaryTag>>(m, "AuxiliaryFunctionRowData");
}

}  // namespace tyr::formalism
