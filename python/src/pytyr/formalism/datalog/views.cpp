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

#include "views.hpp"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <tyr/tyr.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::formalism::datalog
{
namespace
{
template<typename V>
void add_value_semantics(nb::class_<V>& cls)
{
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_object(nb::module_& m)
{
    using V = ObjectView;
    auto cls = nb::class_<V>(m, "Object")  //
                   .def("get_index", &V::get_index)
                   .def("get_name", &V::get_name);
    add_value_semantics(cls);
}

void bind_variable(nb::module_& m)
{
    using V = VariableView;
    auto cls = nb::class_<V>(m, "Variable")  //
                   .def("get_index", &V::get_index)
                   .def("get_name", &V::get_name);
    add_value_semantics(cls);
}

void bind_term(nb::module_& m)
{
    using V = TermView;
    auto cls = nb::class_<V>(m, "Term").def("get_variant", &V::get_variant);
    add_value_semantics(cls);
}

template<typename T>
void bind_relation_binding(nb::module_& m, const std::string& name)
{
    using V = ygg::View<ygg::Index<RelationBinding<T>>, Repository>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_relation", &V::get_relation, nb::keep_alive<0, 1>())
                   .def("get_objects", &V::get_objects)
                   .def("get_key", &V::get_key);
    add_value_semantics(cls);
}

template<FactKind T>
void bind_predicate(nb::module_& m, const std::string& name)
{
    using V = PredicateView<T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_name", &V::get_name)
                   .def("get_arity", &V::get_arity);
    add_value_semantics(cls);
}

template<FactKind T>
void bind_atom(nb::module_& m, const std::string& name)
{
    using V = AtomView<T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_predicate", &V::get_predicate, nb::keep_alive<0, 1>())
                   .def("get_terms", &V::get_terms);
    add_value_semantics(cls);
}

template<FactKind T>
void bind_ground_atom(nb::module_& m, const std::string& name)
{
    using V = GroundAtomView<T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_predicate", &V::get_predicate, nb::keep_alive<0, 1>())
                   .def("get_objects", &V::get_objects)
                   .def("get_key", &V::get_key);
    add_value_semantics(cls);
}

template<FactKind T>
void bind_literal(nb::module_& m, const std::string& name)
{
    using V = LiteralView<T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_atom", &V::get_atom, nb::keep_alive<0, 1>())
                   .def("get_polarity", &V::get_polarity);
    add_value_semantics(cls);
}

template<FactKind T>
void bind_ground_literal(nb::module_& m, const std::string& name)
{
    using V = GroundLiteralView<T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_atom", &V::get_atom, nb::keep_alive<0, 1>())
                   .def("get_polarity", &V::get_polarity);
    add_value_semantics(cls);
}

template<FactKind T>
void bind_function(nb::module_& m, const std::string& name)
{
    using V = FunctionView<T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_name", &V::get_name)
                   .def("get_arity", &V::get_arity);
    add_value_semantics(cls);
}

template<FactKind T>
void bind_function_term(nb::module_& m, const std::string& name)
{
    using V = FunctionTermView<T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_function", &V::get_function, nb::keep_alive<0, 1>())
                   .def("get_terms", &V::get_terms);
    add_value_semantics(cls);
}

template<FactKind T>
void bind_ground_function_term(nb::module_& m, const std::string& name)
{
    using V = GroundFunctionTermView<T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_function", &V::get_function, nb::keep_alive<0, 1>())
                   .def("get_objects", &V::get_objects)
                   .def("get_key", &V::get_key);
    add_value_semantics(cls);
}

template<FactKind T>
void bind_ground_function_term_value(nb::module_& m, const std::string& name)
{
    using V = GroundFunctionTermValueView<T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_fterm", &V::get_fterm, nb::keep_alive<0, 1>())
                   .def("get_value", &V::get_value);
    add_value_semantics(cls);
}

template<OpKind Op, typename T>
void bind_unary_operator(nb::module_& m, const std::string& name)
{
    using V = UnaryOperatorView<Op, T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_arg", &V::get_arg, nb::keep_alive<0, 1>());
    add_value_semantics(cls);
}

template<OpKind Op, typename T>
void bind_binary_operator(nb::module_& m, const std::string& name)
{
    using V = BinaryOperatorView<Op, T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_lhs", &V::get_lhs, nb::keep_alive<0, 1>())
                   .def("get_rhs", &V::get_rhs, nb::keep_alive<0, 1>());
    add_value_semantics(cls);
}

template<OpKind Op, typename T>
void bind_multi_operator(nb::module_& m, const std::string& name)
{
    using V = MultiOperatorView<Op, T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_args", &V::get_args);
    add_value_semantics(cls);
}

template<typename T>
void bind_arithmetic_operator(nb::module_& m, const std::string& name)
{
    using V = ArithmeticOperatorView<T>;
    auto cls = nb::class_<V>(m, name.c_str()).def("get_variant", &V::get_variant);
    add_value_semantics(cls);
}

template<typename T>
void bind_boolean_operator(nb::module_& m, const std::string& name)
{
    using V = BooleanOperatorView<T>;
    auto cls = nb::class_<V>(m, name.c_str()).def("get_variant", &V::get_variant);
    add_value_semantics(cls);
}

template<typename V>
void bind_expression(nb::module_& m, const std::string& name)
{
    auto cls = nb::class_<V>(m, name.c_str()).def("get_variant", &V::get_variant);
    add_value_semantics(cls);
}

template<NumericEffectOpKind Op, FactKind T>
void bind_numeric_effect(nb::module_& m, const std::string& name)
{
    using V = NumericEffectView<Op, T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_fterm", &V::get_fterm, nb::keep_alive<0, 1>())
                   .def("get_fexpr", &V::get_fexpr, nb::keep_alive<0, 1>());
    add_value_semantics(cls);
}

template<NumericEffectOpKind Op, FactKind T>
void bind_ground_numeric_effect(nb::module_& m, const std::string& name)
{
    using V = GroundNumericEffectView<Op, T>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_fterm", &V::get_fterm, nb::keep_alive<0, 1>())
                   .def("get_fexpr", &V::get_fexpr, nb::keep_alive<0, 1>());
    add_value_semantics(cls);
}

template<typename V>
void bind_variant_view(nb::module_& m, const std::string& name)
{
    auto cls = nb::class_<V>(m, name.c_str()).def("get_variant", &V::get_variant);
    add_value_semantics(cls);
}

void bind_conjunctive_condition(nb::module_& m)
{
    using V = ConjunctiveConditionView;
    auto cls = nb::class_<V>(m, "ConjunctiveCondition")  //
                   .def("get_index", &V::get_index)
                   .def("get_variables", &V::get_variables)
                   .def("get_arity", &V::get_arity)
                   .def("get_static_literals", &V::get_literals<StaticTag>)
                   .def("get_fluent_literals", &V::get_literals<FluentTag>)
                   .def("get_numeric_constraints", &V::get_numeric_constraints);
    add_value_semantics(cls);
}

void bind_conjunctive_effect(nb::module_& m)
{
    using V = ConjunctiveEffectView;
    auto cls = nb::class_<V>(m, "ConjunctiveEffect")  //
                   .def("get_index", &V::get_index)
                   .def("get_numeric_effects", &V::get_numeric_effects);
    add_value_semantics(cls);
}

void bind_conditional_effect(nb::module_& m)
{
    using V = ConditionalEffectView;
    auto cls = nb::class_<V>(m, "ConditionalEffect")  //
                   .def("get_index", &V::get_index)
                   .def("get_variables", &V::get_variables)
                   .def("get_condition", &V::get_condition, nb::keep_alive<0, 1>())
                   .def("get_effect", &V::get_effect, nb::keep_alive<0, 1>());
    add_value_semantics(cls);
}

template<RelationKind R>
void bind_rule(nb::module_& m, const std::string& name)
{
    using V = RuleView<R>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_arity", &V::get_arity)
                   .def("get_variables", &V::get_variables)
                   .def("get_body", &V::get_body, nb::keep_alive<0, 1>())
                   .def("get_head", &V::get_head, nb::keep_alive<0, 1>())
                   .def("get_metric_effects", &V::get_metric_effects);
    add_value_semantics(cls);
}

void bind_ground_conjunctive_condition(nb::module_& m)
{
    using V = GroundConjunctiveConditionView;
    auto cls = nb::class_<V>(m, "GroundConjunctiveCondition")  //
                   .def("get_index", &V::get_index)
                   .def("get_static_literals", &V::get_literals<StaticTag>)
                   .def("get_fluent_literals", &V::get_literals<FluentTag>)
                   .def("get_numeric_constraints", &V::get_numeric_constraints);
    add_value_semantics(cls);
}

void bind_ground_conjunctive_effect(nb::module_& m)
{
    using V = GroundConjunctiveEffectView;
    auto cls = nb::class_<V>(m, "GroundConjunctiveEffect")  //
                   .def("get_index", &V::get_index)
                   .def("get_numeric_effects", &V::get_numeric_effects);
    add_value_semantics(cls);
}

void bind_ground_conditional_effect(nb::module_& m)
{
    using V = GroundConditionalEffectView;
    auto cls = nb::class_<V>(m, "GroundConditionalEffect")  //
                   .def("get_index", &V::get_index)
                   .def("get_condition", &V::get_condition, nb::keep_alive<0, 1>())
                   .def("get_effect", &V::get_effect, nb::keep_alive<0, 1>());
    add_value_semantics(cls);
}

template<RelationKind R>
void bind_ground_rule(nb::module_& m, const std::string& name)
{
    using V = GroundRuleView<R>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_rule", &V::get_rule, nb::keep_alive<0, 1>())
                   .def("get_row", &V::get_row)
                   .def("get_objects", &V::get_objects)
                   .def("get_key", &V::get_key)
                   .def("get_body", &V::get_body, nb::keep_alive<0, 1>())
                   .def("get_head", &V::get_head, nb::keep_alive<0, 1>())
                   .def("get_metric_effects", &V::get_metric_effects);
    add_value_semantics(cls);
}

void bind_metric(nb::module_& m)
{
    using V = MetricView;
    auto cls = nb::class_<V>(m, "Metric")  //
                   .def("get_index", &V::get_index)
                   .def("get_fexpr", &V::get_fexpr, nb::keep_alive<0, 1>());
    add_value_semantics(cls);
}

template<TaskKind Kind>
void bind_program(nb::module_& m, const std::string& name)
{
    using V = ProgramView<Kind>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def("get_index", &V::get_index)
                   .def("get_static_predicates", &V::template get_predicates<StaticTag>)
                   .def("get_fluent_predicates", &V::template get_predicates<FluentTag>)
                   .def("get_static_functions", &V::template get_functions<StaticTag>)
                   .def("get_fluent_functions", &V::template get_functions<FluentTag>)
                   .def("get_objects", &V::get_objects)
                   .def("get_static_atoms", &V::template get_atoms<StaticTag>)
                   .def("get_fluent_atoms", &V::template get_atoms<FluentTag>)
                   .def("get_static_fterm_values", &V::template get_fterm_values<StaticTag>)
                   .def("get_fluent_fterm_values", &V::template get_fterm_values<FluentTag>)
                   .def("get_goal", &V::get_goal, nb::keep_alive<0, 1>())
                   .def("get_metric", &V::get_metric);
    if constexpr (std::same_as<Kind, LiftedTag>)
    {
        cls.def("get_rules", &V::template get_rules<PredicateTag>);
        cls.def("get_function_rules", &V::template get_rules<FunctionTag>);
    }
    else
    {
        cls.def("get_ground_rules", &V::template get_ground_rules<PredicateTag>);
        cls.def("get_ground_function_rules", &V::template get_ground_rules<FunctionTag>);
    }
    add_value_semantics(cls);
}
}

void bind_views(nb::module_& m)
{
    bind_object(m);
    bind_variable(m);
    bind_term(m);
    bind_relation_binding<Predicate<StaticTag>>(m, "StaticPredicateBinding");
    bind_relation_binding<Predicate<FluentTag>>(m, "FluentPredicateBinding");
    bind_relation_binding<Function<StaticTag>>(m, "StaticFunctionBinding");
    bind_relation_binding<Function<FluentTag>>(m, "FluentFunctionBinding");
    bind_relation_binding<Function<AuxiliaryTag>>(m, "AuxiliaryFunctionBinding");
    bind_relation_binding<Rule<PredicateTag>>(m, "RuleBinding");
    bind_relation_binding<Rule<FunctionTag>>(m, "FunctionRuleBinding");

    bind_predicate<StaticTag>(m, "StaticPredicate");
    bind_predicate<FluentTag>(m, "FluentPredicate");
    bind_atom<StaticTag>(m, "StaticAtom");
    bind_atom<FluentTag>(m, "FluentAtom");
    bind_ground_atom<StaticTag>(m, "StaticGroundAtom");
    bind_ground_atom<FluentTag>(m, "FluentGroundAtom");
    bind_literal<StaticTag>(m, "StaticLiteral");
    bind_literal<FluentTag>(m, "FluentLiteral");
    bind_ground_literal<StaticTag>(m, "StaticGroundLiteral");
    bind_ground_literal<FluentTag>(m, "FluentGroundLiteral");

    bind_function<StaticTag>(m, "StaticFunction");
    bind_function<FluentTag>(m, "FluentFunction");
    bind_function<AuxiliaryTag>(m, "AuxiliaryFunction");
    bind_function_term<StaticTag>(m, "StaticFunctionTerm");
    bind_function_term<FluentTag>(m, "FluentFunctionTerm");
    bind_function_term<AuxiliaryTag>(m, "AuxiliaryFunctionTerm");
    bind_ground_function_term<StaticTag>(m, "StaticGroundFunctionTerm");
    bind_ground_function_term<FluentTag>(m, "FluentGroundFunctionTerm");
    bind_ground_function_term<AuxiliaryTag>(m, "AuxiliaryGroundFunctionTerm");
    bind_ground_function_term_value<StaticTag>(m, "StaticGroundFunctionTermValue");
    bind_ground_function_term_value<FluentTag>(m, "FluentGroundFunctionTermValue");
    bind_ground_function_term_value<AuxiliaryTag>(m, "AuxiliaryGroundFunctionTermValue");

    bind_unary_operator<Sub, ygg::Data<FunctionExpression>>(m, "UnaryOperatorSub");
    bind_binary_operator<Add, ygg::Data<FunctionExpression>>(m, "BinaryOperatorAdd");
    bind_binary_operator<Sub, ygg::Data<FunctionExpression>>(m, "BinaryOperatorSub");
    bind_binary_operator<Mul, ygg::Data<FunctionExpression>>(m, "BinaryOperatorMul");
    bind_binary_operator<Div, ygg::Data<FunctionExpression>>(m, "BinaryOperatorDiv");
    bind_binary_operator<Eq, ygg::Data<FunctionExpression>>(m, "BinaryOperatorEq");
    bind_binary_operator<Ne, ygg::Data<FunctionExpression>>(m, "BinaryOperatorNe");
    bind_binary_operator<Le, ygg::Data<FunctionExpression>>(m, "BinaryOperatorLe");
    bind_binary_operator<Lt, ygg::Data<FunctionExpression>>(m, "BinaryOperatorLt");
    bind_binary_operator<Ge, ygg::Data<FunctionExpression>>(m, "BinaryOperatorGe");
    bind_binary_operator<Gt, ygg::Data<FunctionExpression>>(m, "BinaryOperatorGt");
    bind_multi_operator<Add, ygg::Data<FunctionExpression>>(m, "MultiOperatorAdd");
    bind_multi_operator<Mul, ygg::Data<FunctionExpression>>(m, "MultiOperatorMul");
    bind_arithmetic_operator<ygg::Data<FunctionExpression>>(m, "ArithmeticOperator");
    bind_boolean_operator<ygg::Data<FunctionExpression>>(m, "BooleanOperator");
    bind_expression<FunctionExpressionView>(m, "FunctionExpression");

    bind_conjunctive_condition(m);
    bind_numeric_effect<Assign, FluentTag>(m, "FluentNumericEffectAssign");
    bind_numeric_effect<Increase, FluentTag>(m, "FluentNumericEffectIncrease");
    bind_numeric_effect<Decrease, FluentTag>(m, "FluentNumericEffectDecrease");
    bind_numeric_effect<ScaleUp, FluentTag>(m, "FluentNumericEffectScaleUp");
    bind_numeric_effect<ScaleDown, FluentTag>(m, "FluentNumericEffectScaleDown");
    bind_variant_view<NumericEffectOperatorView<FluentTag>>(m, "FluentNumericEffectOperator");
    bind_conjunctive_effect(m);
    bind_conditional_effect(m);
    bind_rule<PredicateTag>(m, "Rule");
    bind_rule<FunctionTag>(m, "FunctionRule");

    bind_unary_operator<Sub, ygg::Data<GroundFunctionExpression>>(m, "GroundUnaryOperatorSub");
    bind_binary_operator<Add, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorAdd");
    bind_binary_operator<Sub, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorSub");
    bind_binary_operator<Mul, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorMul");
    bind_binary_operator<Div, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorDiv");
    bind_binary_operator<Eq, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorEq");
    bind_binary_operator<Ne, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorNe");
    bind_binary_operator<Le, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorLe");
    bind_binary_operator<Lt, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorLt");
    bind_binary_operator<Ge, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorGe");
    bind_binary_operator<Gt, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorGt");
    bind_multi_operator<Add, ygg::Data<GroundFunctionExpression>>(m, "GroundMultiOperatorAdd");
    bind_multi_operator<Mul, ygg::Data<GroundFunctionExpression>>(m, "GroundMultiOperatorMul");
    bind_arithmetic_operator<ygg::Data<GroundFunctionExpression>>(m, "GroundArithmeticOperator");
    bind_boolean_operator<ygg::Data<GroundFunctionExpression>>(m, "GroundBooleanOperator");
    bind_expression<GroundFunctionExpressionView>(m, "GroundFunctionExpression");

    bind_ground_conjunctive_condition(m);
    bind_ground_numeric_effect<Assign, FluentTag>(m, "FluentGroundNumericEffectAssign");
    bind_ground_numeric_effect<Increase, FluentTag>(m, "FluentGroundNumericEffectIncrease");
    bind_ground_numeric_effect<Decrease, FluentTag>(m, "FluentGroundNumericEffectDecrease");
    bind_ground_numeric_effect<ScaleUp, FluentTag>(m, "FluentGroundNumericEffectScaleUp");
    bind_ground_numeric_effect<ScaleDown, FluentTag>(m, "FluentGroundNumericEffectScaleDown");
    bind_variant_view<GroundNumericEffectOperatorView<FluentTag>>(m, "FluentGroundNumericEffectOperator");
    bind_ground_conjunctive_effect(m);
    bind_ground_conditional_effect(m);
    bind_ground_rule<PredicateTag>(m, "GroundRule");
    bind_ground_rule<FunctionTag>(m, "GroundFunctionRule");
    bind_metric(m);
    bind_program<LiftedTag>(m, "Program");
    bind_program<GroundTag>(m, "GroundProgram");
}

}
