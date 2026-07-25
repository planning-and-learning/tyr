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

#include <nanobind/stl/optional.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <tyr/tyr.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::formalism::datalog
{
namespace
{
template<typename Tag>
void bind_relation_binding_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<RelationBinding<Tag>>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<ygg::View<ygg::Index<Tag>, Repository>, const ObjectViewList&>(), "relation"_a, "objects"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_atom_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Atom<T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<PredicateView<T>, const TermViewList&>(), "predicate"_a, "terms"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_atom_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundAtom<T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<PredicateBindingView<T>>(), "binding"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_literal_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Literal<T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<AtomView<T>, bool>(), "atom"_a, "polarity"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_literal_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundLiteral<T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<GroundAtomView<T>, bool>(), "atom"_a, "polarity"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_function_term_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<FunctionTerm<T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<FunctionView<T>, const TermViewList&>(), "function"_a, "terms"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_function_term_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundFunctionTerm<T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<FunctionBindingView<T>>(), "binding"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_function_term_value_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundFunctionTermValue<T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<GroundFunctionTermView<T>, ygg::float_t>(), "fterm"_a, "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<OpKind Op, typename T>
void bind_unary_operator_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<UnaryOperator<Op, T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<ygg::View<T, Repository>>(), "arg"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<OpKind Op, typename T>
void bind_binary_operator_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<BinaryOperator<Op, T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<ygg::View<T, Repository>, ygg::View<T, Repository>>(), "lhs"_a, "rhs"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<OpKind Op, typename T>
void bind_multi_operator_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<MultiOperator<Op, T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<const std::vector<ygg::View<T, Repository>>&>(), "args"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<typename T>
void bind_arithmetic_operator_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<ArithmeticOperator<T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<typename V::template ViewVariant<Repository>>(), "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<typename T>
void bind_boolean_operator_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<BooleanOperator<T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<typename V::template ViewVariant<Repository>>(), "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<typename Expression>
void bind_function_expression_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Expression>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<typename V::template ViewVariant<Repository>>(), "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<NumericEffectOpKind Op, FactKind T>
void bind_numeric_effect_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<NumericEffect<Op, T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<FunctionTermView<T>, FunctionExpressionView>(), "fterm"_a, "fexpr"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<NumericEffectOpKind Op, FactKind T>
void bind_ground_numeric_effect_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundNumericEffect<Op, T>>;
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<GroundFunctionTermView<T>, GroundFunctionExpressionView>(), "fterm"_a, "fexpr"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<typename V>
void bind_variant_data(nb::module_& m, const std::string& name)
{
    auto cls = nb::class_<V>(m, name.c_str()).def(nb::init<typename V::template ViewVariant<Repository>>(), "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_conjunctive_condition_data(nb::module_& m)
{
    using V = ygg::Data<ConjunctiveCondition>;
    auto cls =
        nb::class_<V>(m, "ConjunctiveConditionData")  //
            .def(
                nb::init<const VariableViewList&, const LiteralViewList<StaticTag>&, const LiteralViewList<FluentTag>&, const LiftedBooleanOperatorViewList&>(),
                "variables"_a,
                "static_literals"_a,
                "fluent_literals"_a,
                "numeric_constraints"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_conjunctive_effect_data(nb::module_& m)
{
    using V = ygg::Data<ConjunctiveEffect>;
    auto cls = nb::class_<V>(m, "ConjunctiveEffectData").def(nb::init<const NumericEffectOperatorViewList<FluentTag>&>(), "numeric_effects"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_conditional_effect_data(nb::module_& m)
{
    using V = ygg::Data<ConditionalEffect>;
    auto cls = nb::class_<V>(m, "ConditionalEffectData")  //
                   .def(nb::init<const VariableViewList&, ConjunctiveConditionView, ConjunctiveEffectView>(), "variables"_a, "condition"_a, "effect"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<RelationKind R>
void bind_rule_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Rule<R>>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const VariableViewList&,
                                 ConjunctiveConditionView,
                                 typename V::template HeadView<Repository>,
                                 const NumericEffectOperatorViewList<FluentTag>&>(),
                        "variables"_a,
                        "body"_a,
                        "head"_a,
                        "metric_effects"_a = NumericEffectOperatorViewList<FluentTag> {});
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_conjunctive_condition_data(nb::module_& m)
{
    using V = ygg::Data<GroundConjunctiveCondition>;
    auto cls = nb::class_<V>(m, "GroundConjunctiveConditionData")  //
                   .def(nb::init<const GroundLiteralViewList<StaticTag>&, const GroundLiteralViewList<FluentTag>&, const GroundBooleanOperatorViewList&>(),
                        "static_literals"_a,
                        "fluent_literals"_a,
                        "numeric_constraints"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_conjunctive_effect_data(nb::module_& m)
{
    using V = ygg::Data<GroundConjunctiveEffect>;
    auto cls = nb::class_<V>(m, "GroundConjunctiveEffectData")  //
                   .def(nb::init<const GroundNumericEffectOperatorViewList<FluentTag>&>(), "numeric_effects"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_conditional_effect_data(nb::module_& m)
{
    using V = ygg::Data<GroundConditionalEffect>;
    auto cls = nb::class_<V>(m, "GroundConditionalEffectData")  //
                   .def(nb::init<GroundConjunctiveConditionView, GroundConjunctiveEffectView>(), "condition"_a, "effect"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<RelationKind R>
void bind_ground_rule_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundRule<R>>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<RuleBindingView<R>,
                                 GroundConjunctiveConditionView,
                                 typename V::template HeadView<Repository>,
                                 const GroundNumericEffectOperatorViewList<FluentTag>&>(),
                        "binding"_a,
                        "body"_a,
                        "head"_a,
                        "metric_effects"_a = GroundNumericEffectOperatorViewList<FluentTag> {});
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_metric_data(nb::module_& m)
{
    using V = ygg::Data<Metric>;
    auto cls = nb::class_<V>(m, "MetricData").def(nb::init<GroundFunctionExpressionView>(), "fexpr"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<TaskKind Kind>
void bind_program_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<ProgramTag<Kind>>;
    using PredicateRuleViews = std::conditional_t<std::same_as<Kind, LiftedTag>, RuleViewList<PredicateTag>, GroundRuleViewList<PredicateTag>>;
    using FunctionRuleViews = std::conditional_t<std::same_as<Kind, LiftedTag>, RuleViewList<FunctionTag>, GroundRuleViewList<FunctionTag>>;
    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const PredicateViewList<StaticTag>&,
                                 const PredicateViewList<FluentTag>&,
                                 const FunctionViewList<StaticTag>&,
                                 const FunctionViewList<FluentTag>&,
                                 const ObjectViewList&,
                                 const GroundAtomViewList<StaticTag>&,
                                 const GroundAtomViewList<FluentTag>&,
                                 const GroundFunctionTermValueViewList<StaticTag>&,
                                 const GroundFunctionTermValueViewList<FluentTag>&,
                                 const std::optional<GroundConjunctiveConditionView>&,
                                 const std::optional<MetricView>&,
                                 const PredicateRuleViews&,
                                 const FunctionRuleViews&>(),
                        "static_predicates"_a,
                        "fluent_predicates"_a,
                        "static_functions"_a,
                        "fluent_functions"_a,
                        "objects"_a,
                        "static_atoms"_a,
                        "fluent_atoms"_a,
                        "static_fterm_values"_a,
                        "fluent_fterm_values"_a,
                        "goal"_a,
                        "metric"_a,
                        (std::same_as<Kind, LiftedTag> ? "rules"_a : "ground_rules"_a),
                        (std::same_as<Kind, LiftedTag> ? "function_rules"_a : "ground_function_rules"_a));
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}

void bind_datas(nb::module_& m)
{
    bind_relation_binding_data<Rule<PredicateTag>>(m, "RuleBindingData");
    bind_relation_binding_data<Rule<FunctionTag>>(m, "FunctionRuleBindingData");

    bind_atom_data<StaticTag>(m, "StaticAtomData");
    bind_atom_data<FluentTag>(m, "FluentAtomData");
    bind_ground_atom_data<StaticTag>(m, "StaticGroundAtomData");
    bind_ground_atom_data<FluentTag>(m, "FluentGroundAtomData");
    bind_literal_data<StaticTag>(m, "StaticLiteralData");
    bind_literal_data<FluentTag>(m, "FluentLiteralData");
    bind_ground_literal_data<StaticTag>(m, "StaticGroundLiteralData");
    bind_ground_literal_data<FluentTag>(m, "FluentGroundLiteralData");

    bind_function_term_data<StaticTag>(m, "StaticFunctionTermData");
    bind_function_term_data<FluentTag>(m, "FluentFunctionTermData");
    bind_function_term_data<AuxiliaryTag>(m, "AuxiliaryFunctionTermData");
    bind_ground_function_term_data<StaticTag>(m, "StaticGroundFunctionTermData");
    bind_ground_function_term_data<FluentTag>(m, "FluentGroundFunctionTermData");
    bind_ground_function_term_data<AuxiliaryTag>(m, "AuxiliaryGroundFunctionTermData");
    bind_ground_function_term_value_data<StaticTag>(m, "StaticGroundFunctionTermValueData");
    bind_ground_function_term_value_data<FluentTag>(m, "FluentGroundFunctionTermValueData");
    bind_ground_function_term_value_data<AuxiliaryTag>(m, "AuxiliaryGroundFunctionTermValueData");

    bind_unary_operator_data<Sub, ygg::Data<FunctionExpression>>(m, "UnaryOperatorSubData");
    bind_binary_operator_data<Add, ygg::Data<FunctionExpression>>(m, "BinaryOperatorAddData");
    bind_binary_operator_data<Sub, ygg::Data<FunctionExpression>>(m, "BinaryOperatorSubData");
    bind_binary_operator_data<Mul, ygg::Data<FunctionExpression>>(m, "BinaryOperatorMulData");
    bind_binary_operator_data<Div, ygg::Data<FunctionExpression>>(m, "BinaryOperatorDivData");
    bind_binary_operator_data<Eq, ygg::Data<FunctionExpression>>(m, "BinaryOperatorEqData");
    bind_binary_operator_data<Ne, ygg::Data<FunctionExpression>>(m, "BinaryOperatorNeData");
    bind_binary_operator_data<Le, ygg::Data<FunctionExpression>>(m, "BinaryOperatorLeData");
    bind_binary_operator_data<Lt, ygg::Data<FunctionExpression>>(m, "BinaryOperatorLtData");
    bind_binary_operator_data<Ge, ygg::Data<FunctionExpression>>(m, "BinaryOperatorGeData");
    bind_binary_operator_data<Gt, ygg::Data<FunctionExpression>>(m, "BinaryOperatorGtData");
    bind_multi_operator_data<Add, ygg::Data<FunctionExpression>>(m, "MultiOperatorAddData");
    bind_multi_operator_data<Mul, ygg::Data<FunctionExpression>>(m, "MultiOperatorMulData");
    bind_arithmetic_operator_data<ygg::Data<FunctionExpression>>(m, "ArithmeticOperatorData");
    bind_boolean_operator_data<ygg::Data<FunctionExpression>>(m, "BooleanOperatorData");
    bind_function_expression_data<FunctionExpression>(m, "FunctionExpressionData");

    bind_conjunctive_condition_data(m);
    bind_numeric_effect_data<Assign, FluentTag>(m, "FluentNumericEffectAssignData");
    bind_numeric_effect_data<Increase, FluentTag>(m, "FluentNumericEffectIncreaseData");
    bind_numeric_effect_data<Decrease, FluentTag>(m, "FluentNumericEffectDecreaseData");
    bind_numeric_effect_data<ScaleUp, FluentTag>(m, "FluentNumericEffectScaleUpData");
    bind_numeric_effect_data<ScaleDown, FluentTag>(m, "FluentNumericEffectScaleDownData");
    bind_variant_data<ygg::Data<NumericEffectOperator<FluentTag>>>(m, "FluentNumericEffectOperatorData");
    bind_conjunctive_effect_data(m);
    bind_conditional_effect_data(m);
    bind_rule_data<PredicateTag>(m, "RuleData");
    bind_rule_data<FunctionTag>(m, "FunctionRuleData");

    bind_unary_operator_data<Sub, ygg::Data<GroundFunctionExpression>>(m, "GroundUnaryOperatorSubData");
    bind_binary_operator_data<Add, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorAddData");
    bind_binary_operator_data<Sub, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorSubData");
    bind_binary_operator_data<Mul, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorMulData");
    bind_binary_operator_data<Div, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorDivData");
    bind_binary_operator_data<Eq, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorEqData");
    bind_binary_operator_data<Ne, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorNeData");
    bind_binary_operator_data<Le, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorLeData");
    bind_binary_operator_data<Lt, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorLtData");
    bind_binary_operator_data<Ge, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorGeData");
    bind_binary_operator_data<Gt, ygg::Data<GroundFunctionExpression>>(m, "GroundBinaryOperatorGtData");
    bind_multi_operator_data<Add, ygg::Data<GroundFunctionExpression>>(m, "GroundMultiOperatorAddData");
    bind_multi_operator_data<Mul, ygg::Data<GroundFunctionExpression>>(m, "GroundMultiOperatorMulData");
    bind_arithmetic_operator_data<ygg::Data<GroundFunctionExpression>>(m, "GroundArithmeticOperatorData");
    bind_boolean_operator_data<ygg::Data<GroundFunctionExpression>>(m, "GroundBooleanOperatorData");
    bind_function_expression_data<GroundFunctionExpression>(m, "GroundFunctionExpressionData");

    bind_ground_conjunctive_condition_data(m);
    bind_ground_numeric_effect_data<Assign, FluentTag>(m, "FluentGroundNumericEffectAssignData");
    bind_ground_numeric_effect_data<Increase, FluentTag>(m, "FluentGroundNumericEffectIncreaseData");
    bind_ground_numeric_effect_data<Decrease, FluentTag>(m, "FluentGroundNumericEffectDecreaseData");
    bind_ground_numeric_effect_data<ScaleUp, FluentTag>(m, "FluentGroundNumericEffectScaleUpData");
    bind_ground_numeric_effect_data<ScaleDown, FluentTag>(m, "FluentGroundNumericEffectScaleDownData");
    bind_variant_data<ygg::Data<GroundNumericEffectOperator<FluentTag>>>(m, "FluentGroundNumericEffectOperatorData");
    bind_ground_conjunctive_effect_data(m);
    bind_ground_conditional_effect_data(m);
    bind_ground_rule_data<PredicateTag>(m, "GroundRuleData");
    bind_ground_rule_data<FunctionTag>(m, "GroundFunctionRuleData");
    bind_metric_data(m);
    bind_program_data<LiftedTag>(m, "ProgramData");
    bind_program_data<GroundTag>(m, "GroundProgramData");
}

}
