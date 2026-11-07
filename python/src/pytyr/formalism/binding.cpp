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

#include <tyr/formalism/binding_data.hpp>
#include <tyr/formalism/binding_index.hpp>
#include <tyr/formalism/binding_view.hpp>

namespace tyr::formalism
{

namespace
{
template<typename Relation>
void bind_binding_data(nb::module_& m, const char* name)
{
    using namespace nb::literals;
    using V = urd::data<RelationBinding<Relation>>;
    auto cls = nb::class_<V>(m, name)
                   .def(urd::python::init_from_views<urd::index_view<Relation, planning::Repository>, const planning::ObjectViewList&>(), "relation"_a, "objects"_a)
                   .def(urd::python::init_from_views<urd::index_view<Relation, datalog::Repository>, const datalog::ObjectViewList&>(), "relation"_a, "objects"_a);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}
}  // namespace

void bind_binding(nb::module_& m)
{
    {
        using V = ygg::formalism::RelationBindingIndex<Predicate<StaticTag>, ::tyr::formalism::ObjectTag>;
        auto cls = nb::class_<V>(m, "StaticPredicateBindingIndex").def_ro("relation_index", &V::relation).def_ro("row_index", &V::row);
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = ygg::formalism::RelationBindingIndex<Predicate<FluentTag>, ::tyr::formalism::ObjectTag>;
        auto cls = nb::class_<V>(m, "FluentPredicateBindingIndex").def_ro("relation_index", &V::relation).def_ro("row_index", &V::row);
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = ygg::formalism::RelationBindingIndex<Function<StaticTag>, ::tyr::formalism::ObjectTag>;
        auto cls = nb::class_<V>(m, "StaticFunctionBindingIndex").def_ro("relation_index", &V::relation).def_ro("row_index", &V::row);
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = ygg::formalism::RelationBindingIndex<Function<FluentTag>, ::tyr::formalism::ObjectTag>;
        auto cls = nb::class_<V>(m, "FluentFunctionBindingIndex").def_ro("relation_index", &V::relation).def_ro("row_index", &V::row);
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = ygg::formalism::RelationBindingIndex<Function<AuxiliaryTag>, ::tyr::formalism::ObjectTag>;
        auto cls = nb::class_<V>(m, "AuxiliaryFunctionBindingIndex").def_ro("relation_index", &V::relation).def_ro("row_index", &V::row);
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    bind_binding_data<Predicate<StaticTag>>(m, "StaticPredicateBindingData");
    bind_binding_data<Predicate<FluentTag>>(m, "FluentPredicateBindingData");
    bind_binding_data<Function<StaticTag>>(m, "StaticFunctionBindingData");
    bind_binding_data<Function<FluentTag>>(m, "FluentFunctionBindingData");
    bind_binding_data<Function<AuxiliaryTag>>(m, "AuxiliaryFunctionBindingData");
}

}  // namespace tyr::formalism
