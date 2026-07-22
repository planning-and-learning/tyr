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

#include "datas.hpp"

#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <tyr/tyr.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::formalism
{

namespace
{
void bind_object_data(nb::module_& m)
{
    using V = ygg::Data<Object>;

    auto cls = nb::class_<V>(m, "ObjectData")  //
                   .def(nb::init<const std::string&>(), "name"_a)
                   .def_rw("name", &V::name);
    ygg::add_comparison(cls);
}

void bind_variable_data(nb::module_& m)
{
    using V = ygg::Data<Variable>;

    auto cls = nb::class_<V>(m, "VariableData")  //
                   .def(nb::init<const std::string&>(), "name"_a)
                   .def_rw("name", &V::name);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_term_data(nb::module_& m)
{
    using V = ygg::Data<Term>;

    auto cls = nb::class_<V>(m, "TermData")  //
                   .def(nb::init<typename V::template ViewVariant<planning::Repository>>(), "value"_a)
                   .def(nb::init<typename V::template ViewVariant<datalog::Repository>>(), "value"_a)
                   .def_rw("value", &V::value);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<typename Tag>
void bind_relation_binding_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<RelationBinding<Tag>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<ygg::View<ygg::Index<Tag>, planning::Repository>, const planning::ObjectViewList&>(), "relation"_a, "objects"_a)
                   .def(nb::init<ygg::View<ygg::Index<Tag>, datalog::Repository>, const datalog::ObjectViewList&>(), "relation"_a, "objects"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_predicate_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Predicate<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const std::string&, ygg::uint_t>(), "name"_a, "arity"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_function_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Function<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const std::string&, ygg::uint_t>(), "name"_a, "arity"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}

void bind_datas(nb::module_& m)
{
    bind_object_data(m);
    bind_variable_data(m);
    bind_term_data(m);

    bind_relation_binding_data<Predicate<StaticTag>>(m, "StaticPredicateBindingData");
    bind_relation_binding_data<Predicate<FluentTag>>(m, "FluentPredicateBindingData");
    bind_relation_binding_data<Function<StaticTag>>(m, "StaticFunctionBindingData");
    bind_relation_binding_data<Function<FluentTag>>(m, "FluentFunctionBindingData");
    bind_relation_binding_data<Function<AuxiliaryTag>>(m, "AuxiliaryFunctionBindingData");

    bind_predicate_data<StaticTag>(m, "StaticPredicateData");
    bind_predicate_data<FluentTag>(m, "FluentPredicateData");
    bind_function_data<StaticTag>(m, "StaticFunctionData");
    bind_function_data<FluentTag>(m, "FluentFunctionData");
    bind_function_data<AuxiliaryTag>(m, "AuxiliaryFunctionData");
}

}
