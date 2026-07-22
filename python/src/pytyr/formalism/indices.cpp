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

#include "indices.hpp"

#include <tyr/tyr.hpp>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::formalism
{
using ygg::bind_index;

namespace
{
template<typename Tag>
void bind_relation_binding_index(nb::module_& m, const std::string& name)
{
    using V = ygg::Index<RelationBinding<Tag>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def_ro("relation_index", &V::relation)
                   .def_ro("row_index", &V::row);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}

void bind_indices(nb::module_& m)
{
    ygg::bind_fixed_uint<ParameterIndex>(m, "ParameterIndex");
    bind_index<ygg::Index<Row>>(m, "RowIndex");
    bind_index<ygg::Index<Object>>(m, "ObjectIndex");
    bind_index<ygg::Index<Variable>>(m, "VariableIndex");

    bind_relation_binding_index<Predicate<StaticTag>>(m, "StaticPredicateBindingIndex");
    bind_relation_binding_index<Predicate<FluentTag>>(m, "FluentPredicateBindingIndex");
    bind_relation_binding_index<Function<StaticTag>>(m, "StaticFunctionBindingIndex");
    bind_relation_binding_index<Function<FluentTag>>(m, "FluentFunctionBindingIndex");
    bind_relation_binding_index<Function<AuxiliaryTag>>(m, "AuxiliaryFunctionBindingIndex");

    bind_index<ygg::Index<Predicate<StaticTag>>>(m, "StaticPredicateIndex");
    bind_index<ygg::Index<Predicate<FluentTag>>>(m, "FluentPredicateIndex");
    bind_index<ygg::Index<Function<StaticTag>>>(m, "StaticFunctionIndex");
    bind_index<ygg::Index<Function<FluentTag>>>(m, "FluentFunctionIndex");
    bind_index<ygg::Index<Function<AuxiliaryTag>>>(m, "AuxiliaryFunctionIndex");
}

}
