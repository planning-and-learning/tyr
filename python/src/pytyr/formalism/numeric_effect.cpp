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
#include "datalog/binding_utils.hpp"
#include "planning/binding_utils.hpp"
#include "pytyr/module.hpp"

#include <tyr/formalism/numeric_effect_data.hpp>
#include <tyr/formalism/numeric_effect_view.hpp>

/// Numeric effects: their indices and data are the same for datalog and planning, bound once here (and built from
/// either's views); their views are each repository's (datalog has fluent ones only).
namespace tyr::formalism
{

namespace
{
template<TaskKind T, FactKind F>
void bind_numeric_effect_data(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    urd::python::bind_index<urd::index<NumericEffect<T, F>>>(m, name + "Index");

    using V = urd::data<NumericEffect<T, F>>;
    auto cls = nb::class_<V>(m, (name + "Data").c_str());
    cls.def(urd::python::init_from_views<NumericEffectOperatorKind, datalog::FunctionTermView<T, F>, datalog::FunctionExpressionView<T>>(),
            "operator"_a,
            "fterm"_a,
            "fexpr"_a);
    cls.def(urd::python::init_from_views<NumericEffectOperatorKind, planning::FunctionTermView<T, F>, planning::FunctionExpressionView<T>>(),
            "operator"_a,
            "fterm"_a,
            "fexpr"_a);
    cls.def(nb::init<NumericEffectOperatorKind, urd::index<FunctionTerm<T, F>>, FunctionExpression<T>>(), "operator"_a, "fterm"_a, "fexpr"_a);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);
}

template<TaskKind T, FactKind F, typename C>
void bind_numeric_effect_view(nb::module_& m, nb::class_<C>& repository, const std::string& name)
{
    using V = urd::index_view<NumericEffect<T, F>, C>;
    auto cls = nb::class_<V>(m, name.c_str());
    cls.def("index", &V::index);
    cls.def("get_operator", &V::get_operator);
    cls.def("get_fterm", &V::get_fterm);
    cls.def("get_fexpr", &V::get_fexpr);
    ygg::add_print(cls);
    urd::python::add_comparison(cls);
    urd::python::add_hash(cls);

    ygg::bind_insert<NumericEffect<T, F>>(repository);
}
}  // namespace

void bind_numeric_effect(nb::module_& m)
{
    bind_numeric_effect_data<LiftedTag, FluentTag>(m, "FluentNumericEffect");
    bind_numeric_effect_data<LiftedTag, AuxiliaryTag>(m, "AuxiliaryNumericEffect");
    bind_numeric_effect_data<GroundTag, FluentTag>(m, "FluentGroundNumericEffect");
    bind_numeric_effect_data<GroundTag, AuxiliaryTag>(m, "AuxiliaryGroundNumericEffect");
}

}  // namespace tyr::formalism

namespace tyr::formalism::datalog
{

void bind_numeric_effect(nb::module_& m, RepositoryBinding& repository)
{
    bind_numeric_effect_view<LiftedTag, FluentTag>(m, repository, "FluentNumericEffect");
    bind_numeric_effect_view<GroundTag, FluentTag>(m, repository, "FluentGroundNumericEffect");
}

}  // namespace tyr::formalism::datalog

namespace tyr::formalism::planning
{

void bind_numeric_effect(nb::module_& m, RepositoryBinding& repository)
{
    bind_numeric_effect_view<LiftedTag, FluentTag>(m, repository, "FluentNumericEffect");
    bind_numeric_effect_view<LiftedTag, AuxiliaryTag>(m, repository, "AuxiliaryNumericEffect");
    bind_numeric_effect_view<GroundTag, FluentTag>(m, repository, "FluentGroundNumericEffect");
    bind_numeric_effect_view<GroundTag, AuxiliaryTag>(m, repository, "AuxiliaryGroundNumericEffect");
}

}  // namespace tyr::formalism::planning
