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

#include "module.hpp"

#include "bindings.hpp"
#include "datalog/module.hpp"
#include "planning/module.hpp"

namespace tyr::formalism
{
namespace
{
void alias_shared_types(nb::module_& source, nb::module_& target)
{
    for (const auto* name : { "ParameterIndex",
                              "RowIndex",
                              "ObjectIndex",
                              "ObjectData",
                              "VariableIndex",
                              "VariableData",
                              "TermData",
                              "StaticPredicateBindingIndex",
                              "StaticPredicateBindingData",
                              "FluentPredicateBindingIndex",
                              "FluentPredicateBindingData",
                              "StaticFunctionBindingIndex",
                              "StaticFunctionBindingData",
                              "FluentFunctionBindingIndex",
                              "FluentFunctionBindingData",
                              "AuxiliaryFunctionBindingIndex",
                              "AuxiliaryFunctionBindingData",
                              "StaticPredicateIndex",
                              "StaticPredicateData",
                              "FluentPredicateIndex",
                              "FluentPredicateData",
                              "StaticFunctionIndex",
                              "StaticFunctionData",
                              "FluentFunctionIndex",
                              "FluentFunctionData",
                              "AuxiliaryFunctionIndex",
                              "AuxiliaryFunctionData" })
    {
        target.attr(name) = source.attr(name);
    }
}
}

void bind_module_definitions(nb::module_& m)
{
    bind_formalism(m);

    auto planning_module = m.def_submodule("planning");
    planning::bind_module_definitions(planning_module);
    alias_shared_types(m, planning_module);

    auto datalog_module = m.def_submodule("datalog");
    datalog::bind_module_definitions(datalog_module);
    alias_shared_types(m, datalog_module);
}

}  // namespace tyr::formalism
