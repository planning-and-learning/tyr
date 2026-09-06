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

#include <tyr/formalism/datalog/boolean_operator_data.hpp>
#include <tyr/formalism/datalog/boolean_operator_view.hpp>

namespace tyr::formalism::datalog
{

namespace
{
template<TaskKind T>
void bind_boolean_operator_kind(nb::module_& m, RepositoryBinding& repository, const std::string& name)
{
    using Tag = BooleanOperator<T>;

    {
        using V = ygg::Data<Tag>;
        auto cls = nb::class_<V>(m, (name + "Data").c_str()).def(nb::init<typename V::template ViewVariant<Repository>>(), "value"_a);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    {
        using V = BooleanOperatorView<T>;
        auto cls = nb::class_<V>(m, name.c_str()).def("get_variant", &V::get_variant);
        ygg::add_print(cls);
        ygg::add_comparison(cls);
        ygg::add_hash(cls);
    }

    repository.def("create", &create_data<Tag>, "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>());
}
}  // namespace

void bind_boolean_operator(nb::module_& m, RepositoryBinding& repository)
{
    bind_boolean_operator_kind<LiftedTag>(m, repository, "BooleanOperator");
    bind_boolean_operator_kind<GroundTag>(m, repository, "GroundBooleanOperator");
}

}  // namespace tyr::formalism::datalog
