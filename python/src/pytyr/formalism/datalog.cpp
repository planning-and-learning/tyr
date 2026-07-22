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

#include "datalog/datas.hpp"
#include "datalog/indices.hpp"
#include "datalog/module.hpp"
#include "datalog/views.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/tyr.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::formalism::datalog
{
namespace
{
template<typename T>
auto bind_get_or_create_canonical()
{
    return [](Repository& self, ygg::Data<T>& data)
    {
        canonicalize(data);
        return self.template get_or_create<T>(data).first;
    };
}

template<typename T>
auto bind_get_or_create_relation()
{
    return [](Repository& self, const ygg::Data<RelationBinding<T>>& data) { return self.template get_or_create<T>(data).first; };
}

template<typename T>
auto bind_create()
{
    return [](Repository& self, const ygg::Data<T>& data) { return make_view(data, self); };
}

template<typename T>
void bind_repository_type(nb::class_<Repository>& cls)
{
    cls.def("get_or_create", bind_get_or_create_canonical<T>(), "data"_a, nb::keep_alive<0, 1>());
}
}

void bind_module_definitions(nb::module_& m)
{
    bind_indices(m);
    bind_datas(m);
    bind_views(m);

    nb::class_<RepositoryFactory>(m, "RepositoryFactory")  //
        .def(nb::new_([]() { return std::make_shared<RepositoryFactory>(); }))
        .def("create_repository", &RepositoryFactory::create_shared, "parent_repository"_a = nullptr, nb::keep_alive<0, 2>());

    auto cls =
        nb::class_<Repository>(m, "Repository")  //
            .def("create", bind_create<Term>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
            .def("create", bind_create<FunctionExpression>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
            .def("create", bind_create<GroundFunctionExpression>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
            .def("create", bind_create<BooleanOperator<ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
            .def("create", bind_create<BooleanOperator<ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
            .def("create", bind_create<ArithmeticOperator<ygg::Data<FunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
            .def("create", bind_create<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
            .def("create", bind_create<NumericEffectOperator<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())
            .def("create", bind_create<GroundNumericEffectOperator<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>(), nb::keep_alive<0, 2>())

            .def("get_or_create", bind_get_or_create_canonical<Object>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<Variable>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_relation<Predicate<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_relation<Predicate<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_relation<Function<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_relation<Function<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_relation<Function<AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_relation<Rule>(), "data"_a, nb::keep_alive<0, 1>())

            .def("get_or_create", bind_get_or_create_canonical<Predicate<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<Predicate<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<Atom<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<Atom<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<GroundAtom<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<GroundAtom<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<Literal<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<Literal<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<GroundLiteral<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<GroundLiteral<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<Function<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<Function<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<Function<AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<FunctionTerm<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<FunctionTerm<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<FunctionTerm<AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<GroundFunctionTerm<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<GroundFunctionTerm<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<GroundFunctionTerm<AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<GroundFunctionTermValue<StaticTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<GroundFunctionTermValue<FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
            .def("get_or_create", bind_get_or_create_canonical<GroundFunctionTermValue<AuxiliaryTag>>(), "data"_a, nb::keep_alive<0, 1>());

    bind_repository_type<UnaryOperator<Sub, ygg::Data<FunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Add, ygg::Data<FunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Sub, ygg::Data<FunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Mul, ygg::Data<FunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Div, ygg::Data<FunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Eq, ygg::Data<FunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Ne, ygg::Data<FunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Le, ygg::Data<FunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Lt, ygg::Data<FunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Ge, ygg::Data<FunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Gt, ygg::Data<FunctionExpression>>>(cls);
    bind_repository_type<MultiOperator<Add, ygg::Data<FunctionExpression>>>(cls);
    bind_repository_type<MultiOperator<Mul, ygg::Data<FunctionExpression>>>(cls);

    cls.def("get_or_create", bind_get_or_create_canonical<NumericEffect<Assign, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<NumericEffect<Increase, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<NumericEffect<Decrease, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<NumericEffect<ScaleUp, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<NumericEffect<ScaleDown, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<ConjunctiveCondition>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<ConjunctiveEffect>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<ConditionalEffect>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<Rule>(), "data"_a, nb::keep_alive<0, 1>());

    bind_repository_type<UnaryOperator<Sub, ygg::Data<GroundFunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Add, ygg::Data<GroundFunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Sub, ygg::Data<GroundFunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Mul, ygg::Data<GroundFunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Div, ygg::Data<GroundFunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Eq, ygg::Data<GroundFunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Ne, ygg::Data<GroundFunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Le, ygg::Data<GroundFunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Lt, ygg::Data<GroundFunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Ge, ygg::Data<GroundFunctionExpression>>>(cls);
    bind_repository_type<BinaryOperator<Gt, ygg::Data<GroundFunctionExpression>>>(cls);
    bind_repository_type<MultiOperator<Add, ygg::Data<GroundFunctionExpression>>>(cls);
    bind_repository_type<MultiOperator<Mul, ygg::Data<GroundFunctionExpression>>>(cls);

    cls.def("get_or_create", bind_get_or_create_canonical<GroundNumericEffect<Assign, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<GroundNumericEffect<Increase, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<GroundNumericEffect<Decrease, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<GroundNumericEffect<ScaleUp, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<GroundNumericEffect<ScaleDown, FluentTag>>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<GroundConjunctiveCondition>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<GroundConjunctiveEffect>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<GroundConditionalEffect>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<GroundRule>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<Metric>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<Program>(), "data"_a, nb::keep_alive<0, 1>())
        .def("get_or_create", bind_get_or_create_canonical<GroundProgram>(), "data"_a, nb::keep_alive<0, 1>());
}

}
