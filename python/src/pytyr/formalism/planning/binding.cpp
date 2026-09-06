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

#include <tyr/formalism/binding_data.hpp>
#include <tyr/formalism/binding_index.hpp>
#include <tyr/formalism/binding_view.hpp>

namespace tyr::formalism::planning
{

namespace
{
template<typename Relation>
void bind_binding_data(nb::module_& m, const char* name)
{
    using V = ygg::Data<RelationBinding<Relation>>;
    auto cls = nb::class_<V>(m, name).def(nb::init<ygg::View<ygg::Index<Relation>, Repository>, const ObjectViewList&>(), "relation"_a, "objects"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<typename Relation>
void bind_binding_view(nb::module_& m, const char* name)
{
    using V = ygg::View<ygg::Index<RelationBinding<Relation>>, Repository>;
    auto cls = nb::class_<V>(m, name)
                   .def("get_index", &V::get_index)
                   .def("get_relation", &V::get_relation, nb::keep_alive<0, 1>())
                   .def("get_objects", &V::get_objects);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_binding(nb::module_& m, RepositoryBinding& repository)
{
    {
        using V = ygg::Index<RelationBinding<Predicate<DerivedTag>>>;
        auto cls = nb::class_<V>(m, "DerivedPredicateBindingIndex").def_ro("relation_index", &V::relation).def_ro("row_index", &V::row);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = ygg::Index<RelationBinding<Action<::tyr::LiftedTag>>>;
        auto cls = nb::class_<V>(m, "ActionBindingIndex").def_ro("relation_index", &V::relation).def_ro("row_index", &V::row);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = ygg::Index<RelationBinding<Axiom<::tyr::LiftedTag>>>;
        auto cls = nb::class_<V>(m, "AxiomBindingIndex").def_ro("relation_index", &V::relation).def_ro("row_index", &V::row);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    bind_binding_data<Predicate<DerivedTag>>(m, "DerivedPredicateBindingData");
    bind_binding_data<Action<::tyr::LiftedTag>>(m, "ActionBindingData");
    bind_binding_data<Axiom<::tyr::LiftedTag>>(m, "AxiomBindingData");

    bind_binding_view<Predicate<DerivedTag>>(m, "DerivedPredicateBinding");
    bind_binding_view<Action<::tyr::LiftedTag>>(m, "ActionBinding");
    bind_binding_view<Axiom<::tyr::LiftedTag>>(m, "AxiomBinding");
    bind_binding_view<Predicate<StaticTag>>(m, "StaticPredicateBinding");
    bind_binding_view<Predicate<FluentTag>>(m, "FluentPredicateBinding");
    bind_binding_view<Function<StaticTag>>(m, "StaticFunctionBinding");
    bind_binding_view<Function<FluentTag>>(m, "FluentFunctionBinding");
    bind_binding_view<Function<AuxiliaryTag>>(m, "AuxiliaryFunctionBinding");

    repository.def("get_or_create", &get_or_create_relation_data<Predicate<DerivedTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_relation_data<Action<::tyr::LiftedTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_relation_data<Axiom<::tyr::LiftedTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_relation_data<Predicate<StaticTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_relation_data<Predicate<FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_relation_data<Function<StaticTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_relation_data<Function<FluentTag>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_relation_data<Function<AuxiliaryTag>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::planning
