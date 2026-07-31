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

#include <tyr/formalism/planning/unary_operator_data.hpp>
#include <tyr/formalism/planning/unary_operator_index.hpp>
#include <tyr/formalism/planning/unary_operator_view.hpp>

namespace tyr::formalism::planning
{

namespace
{
template<typename Expression>
void bind_unary_operator_data(nb::module_& m, const char* name)
{
    using ExpressionData = ygg::Data<Expression>;
    using ExpressionView = ygg::View<ExpressionData, Repository>;
    using Tag = UnaryOperator<Sub, ExpressionData>;

    using V = ygg::Data<Tag>;
    auto cls = nb::class_<V>(m, name).def(nb::init<ExpressionView>(), "arg"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<typename Expression>
void bind_unary_operator_view(nb::module_& m, const char* name)
{
    using ExpressionData = ygg::Data<Expression>;

    using V = UnaryOperatorView<Sub, ExpressionData>;
    auto cls = nb::class_<V>(m, name).def("get_index", &V::get_index).def("get_arg", &V::get_arg);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}  // namespace

void bind_unary_operator(nb::module_& m, RepositoryBinding& repository)
{
    ygg::bind_index<ygg::Index<UnaryOperator<Sub, ygg::Data<FunctionExpression>>>>(m, "UnaryOperatorSubIndex");
    ygg::bind_index<ygg::Index<UnaryOperator<Sub, ygg::Data<GroundFunctionExpression>>>>(m, "GroundUnaryOperatorSubIndex");

    bind_unary_operator_data<FunctionExpression>(m, "UnaryOperatorSubData");
    bind_unary_operator_data<GroundFunctionExpression>(m, "GroundUnaryOperatorSubData");

    bind_unary_operator_view<FunctionExpression>(m, "UnaryOperatorSub");
    bind_unary_operator_view<GroundFunctionExpression>(m, "GroundUnaryOperatorSub");

    repository.def("get_or_create", &get_or_create_data<UnaryOperator<Sub, ygg::Data<FunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
    repository.def("get_or_create", &get_or_create_data<UnaryOperator<Sub, ygg::Data<GroundFunctionExpression>>>, "data"_a, nb::keep_alive<0, 1>());
}

}  // namespace tyr::formalism::planning
