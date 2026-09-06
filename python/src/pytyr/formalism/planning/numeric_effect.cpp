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

#include <tyr/formalism/planning/numeric_effect_data.hpp>
#include <tyr/formalism/planning/numeric_effect_index.hpp>
#include <tyr/formalism/planning/numeric_effect_view.hpp>

namespace tyr::formalism::planning
{

namespace
{
template<TaskKind T, FactKind F>
void bind_numeric_effect_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using Tag = NumericEffect<T, F>;
    ygg::bind_index<ygg::Index<Tag>>(m, (name + "Index").c_str());

    {
        using V = ygg::Data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str());
        cls.def(nb::init<NumericEffectOperatorKind, FunctionTermView<T, F>, FunctionExpressionView<T>>(), "operator"_a, "fterm"_a, "fexpr"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = NumericEffectView<T, F>;
        auto cls = nb::class_<V>(m, name.c_str());
        cls.def("get_index", &V::get_index);
        cls.def("get_operator", &V::get_operator);
        cls.def("get_fterm", &V::get_fterm, nb::keep_alive<0, 1>());
        cls.def("get_fexpr", &V::get_fexpr, nb::keep_alive<0, 1>());
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("get_or_create", &get_or_create_data<Tag>, "data"_a, nb::keep_alive<0, 1>());
}
}  // namespace

void bind_numeric_effect(nb::module_& m, RepositoryBinding& repository)
{
    bind_numeric_effect_kind<LiftedTag, FluentTag>(m, repository, "FluentNumericEffect");
    bind_numeric_effect_kind<LiftedTag, AuxiliaryTag>(m, repository, "AuxiliaryNumericEffect");
    bind_numeric_effect_kind<GroundTag, FluentTag>(m, repository, "FluentGroundNumericEffect");
    bind_numeric_effect_kind<GroundTag, AuxiliaryTag>(m, repository, "AuxiliaryGroundNumericEffect");
}

}  // namespace tyr::formalism::planning
