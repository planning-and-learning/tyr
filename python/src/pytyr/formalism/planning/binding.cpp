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

#include <tyr/formalism/declarations.hpp>
#include <yggdrasil/formalism/binding_data.hpp>
#include <tyr/formalism/binding_index.hpp>
#include <yggdrasil/formalism/binding_view.hpp>

namespace tyr::formalism::planning
{

namespace
{
template<typename Relation>
void bind_binding_data(nb::module_& m, const char* name)
{
    using namespace nb::literals;
    using V = urd::data<RelationBinding<Relation>>;
    auto cls = nb::class_<V>(m, name).def(urd::python::init_from_views<urd::index_view<Relation, Repository>, const ObjectViewList&>(), "relation"_a, "objects"_a);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}

template<typename Relation>
void bind_binding_view(nb::module_& m, const char* name)
{
    using V = ::ygg::formalism::RelationBindingView<ygg::formalism::RelationBindingIndex<Relation, ::tyr::formalism::ObjectTag>, Repository>;
    auto cls = nb::class_<V>(m, name)
                   .def("index", [](const V& self) { return self.data(); })
                   .def("get_relation", &V::get_relation)
                   .def("get_objects", [](const V& self) { return self.get_objects(); });
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}
}  // namespace

void bind_binding(nb::module_& m, RepositoryBinding& repository)
{
    {
        using V = ygg::formalism::RelationBindingIndex<Predicate<DerivedTag>, ::tyr::formalism::ObjectTag>;
        auto cls = nb::class_<V>(m, "DerivedPredicateBindingIndex").def_ro("relation_index", &V::relation).def_ro("row_index", &V::row);
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = ygg::formalism::RelationBindingIndex<Action<LiftedTag>, ::tyr::formalism::ObjectTag>;
        auto cls = nb::class_<V>(m, "ActionBindingIndex").def_ro("relation_index", &V::relation).def_ro("row_index", &V::row);
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    {
        using V = ygg::formalism::RelationBindingIndex<Axiom<LiftedTag>, ::tyr::formalism::ObjectTag>;
        auto cls = nb::class_<V>(m, "AxiomBindingIndex").def_ro("relation_index", &V::relation).def_ro("row_index", &V::row);
        ygg::add_print(cls);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
    }

    bind_binding_data<Predicate<DerivedTag>>(m, "DerivedPredicateBindingData");
    bind_binding_data<Action<LiftedTag>>(m, "ActionBindingData");
    bind_binding_data<Axiom<LiftedTag>>(m, "AxiomBindingData");

    bind_binding_view<Predicate<DerivedTag>>(m, "DerivedPredicateBinding");
    bind_binding_view<Action<LiftedTag>>(m, "ActionBinding");
    bind_binding_view<Axiom<LiftedTag>>(m, "AxiomBinding");
    bind_binding_view<Predicate<StaticTag>>(m, "StaticPredicateBinding");
    bind_binding_view<Predicate<FluentTag>>(m, "FluentPredicateBinding");
    bind_binding_view<Function<StaticTag>>(m, "StaticFunctionBinding");
    bind_binding_view<Function<FluentTag>>(m, "FluentFunctionBinding");
    bind_binding_view<Function<AuxiliaryTag>>(m, "AuxiliaryFunctionBinding");

    ygg::bind_insert<RelationBinding<Predicate<DerivedTag>>>(repository);
    ygg::bind_insert<RelationBinding<Action<LiftedTag>>>(repository);
    ygg::bind_insert<RelationBinding<Axiom<LiftedTag>>>(repository);
    ygg::bind_insert<RelationBinding<Predicate<StaticTag>>>(repository);
    ygg::bind_insert<RelationBinding<Predicate<FluentTag>>>(repository);
    ygg::bind_insert<RelationBinding<Function<StaticTag>>>(repository);
    ygg::bind_insert<RelationBinding<Function<FluentTag>>>(repository);
    ygg::bind_insert<RelationBinding<Function<AuxiliaryTag>>>(repository);
}

}  // namespace tyr::formalism::planning
