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

#include <tyr/formalism/function_data.hpp>
#include <tyr/formalism/function_index.hpp>
#include <tyr/formalism/function_view.hpp>

namespace tyr::formalism
{

namespace
{
template<FactKind T>
void bind_function_data(nb::module_& m, const char* name)
{
    using V = ygg::Data<Function<T>>;
    auto cls = nb::class_<V>(m, name).def(nb::init<const std::string&, ygg::uint_t>(), "name"_a, "arity"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_function(nb::module_& m)
{
    ygg::bind_index<ygg::Index<Function<StaticTag>>>(m, "StaticFunctionIndex");
    ygg::bind_index<ygg::Index<Function<FluentTag>>>(m, "FluentFunctionIndex");
    ygg::bind_index<ygg::Index<Function<AuxiliaryTag>>>(m, "AuxiliaryFunctionIndex");

    bind_function_data<StaticTag>(m, "StaticFunctionData");
    bind_function_data<FluentTag>(m, "FluentFunctionData");
    bind_function_data<AuxiliaryTag>(m, "AuxiliaryFunctionData");
}

}  // namespace tyr::formalism
