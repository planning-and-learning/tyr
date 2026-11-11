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

namespace tyr::formalism::planning
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
    auto cls = nb::class_<V>(m, name).def("__init__", &init_row_data<V, urd::index_view<Relation, Repository>, ObjectViewList>, "relation"_a, "objects"_a);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}

template<typename Relation>
void bind_row_view(nb::module_& m, const char* name)
{
    using V = ygg::RowView<Relation, Repository>;
    auto cls = nb::class_<V>(m, name)
                   .def("index", [](const V& self) { return self.data(); })
                   .def("get_relation", &V::get_relation)
                   .def("get_objects", [](const V& self) { return urd::resolve(self.template get<urd::index<Object>>(), self.context()); });
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}
}  // namespace

void bind_row(nb::module_& m, RepositoryBinding& repository)
{
    {
        using V = ygg::RowIndex<Predicate<DerivedTag>>;
        auto cls = nb::class_<V>(m, "DerivedPredicateRowIndex")
                       .def_ro("relation_index", &V::relation)
                       .def_prop_ro("row_index", [](const V& self) { return self.row.value; });
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = ygg::RowIndex<Action<LiftedTag>>;
        auto cls =
            nb::class_<V>(m, "ActionRowIndex").def_ro("relation_index", &V::relation).def_prop_ro("row_index", [](const V& self) { return self.row.value; });
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = ygg::RowIndex<Axiom<LiftedTag>>;
        auto cls =
            nb::class_<V>(m, "AxiomRowIndex").def_ro("relation_index", &V::relation).def_prop_ro("row_index", [](const V& self) { return self.row.value; });
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    bind_row_data<Predicate<DerivedTag>>(m, "DerivedPredicateRowData");
    bind_row_data<Action<LiftedTag>>(m, "ActionRowData");
    bind_row_data<Axiom<LiftedTag>>(m, "AxiomRowData");

    bind_row_view<Predicate<DerivedTag>>(m, "DerivedPredicateRow");
    bind_row_view<Action<LiftedTag>>(m, "ActionRow");
    bind_row_view<Axiom<LiftedTag>>(m, "AxiomRow");
    bind_row_view<Predicate<StaticTag>>(m, "StaticPredicateRow");
    bind_row_view<Predicate<FluentTag>>(m, "FluentPredicateRow");
    bind_row_view<Function<StaticTag>>(m, "StaticFunctionRow");
    bind_row_view<Function<FluentTag>>(m, "FluentFunctionRow");
    bind_row_view<Function<AuxiliaryTag>>(m, "AuxiliaryFunctionRow");

    ygg::bind_insert<ygg::Row<Predicate<DerivedTag>>>(repository);
    ygg::bind_insert<ygg::Row<Action<LiftedTag>>>(repository);
    ygg::bind_insert<ygg::Row<Axiom<LiftedTag>>>(repository);
    ygg::bind_insert<ygg::Row<Predicate<StaticTag>>>(repository);
    ygg::bind_insert<ygg::Row<Predicate<FluentTag>>>(repository);
    ygg::bind_insert<ygg::Row<Function<StaticTag>>>(repository);
    ygg::bind_insert<ygg::Row<Function<FluentTag>>>(repository);
    ygg::bind_insert<ygg::Row<Function<AuxiliaryTag>>>(repository);
}

}  // namespace tyr::formalism::planning
