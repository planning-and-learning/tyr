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
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <tyr/tyr.hpp>
#include <yggdrasil/python/bindings.hpp>
#include <yggdrasil/python/type_casters.hpp>

namespace tyr::formalism::planning
{

namespace
{

/**
 * ygg::Data
 */

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
void bind_predicate_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Predicate<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const std::string&, ygg::uint_t>(), "name"_a, "arity"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_atom_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Atom<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<PredicateView<T>, const TermViewList&>(), "predicate"_a, "terms"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_atom_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundAtom<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<PredicateBindingView<T>>(), "binding"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_literal_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Literal<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<AtomView<T>, bool>(), "atom"_a, "polarity"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_literal_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundLiteral<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<GroundAtomView<T>, bool>(), "atom"_a, "polarity"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_fdr_variable_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<FDRVariable<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const GroundAtomViewList<T>>(), "atoms"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_fdr_fact_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<FDRFact<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<FDRVariableView<T>, FDRValue>(), "variable"_a, "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_function_term_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<FunctionTerm<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<FunctionView<T>, const TermViewList&>(), "function"_a, "terms"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_function_term_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundFunctionTerm<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<FunctionBindingView<T>>(), "binding"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_function_term_value_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundFunctionTermValue<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<GroundFunctionTermView<T>, ygg::float_t>(), "fterm"_a, "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<NumericEffectOpKind Op, FactKind T>
void bind_numeric_effect_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<NumericEffect<Op, T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<FunctionTermView<T>, FunctionExpressionView>(), "fterm"_a, "fexpr"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<NumericEffectOpKind Op, FactKind T>
void bind_ground_numeric_effect_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundNumericEffect<Op, T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<GroundFunctionTermView<T>, GroundFunctionExpressionView>(), "fterm"_a, "fexpr"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_numeric_effect_operator_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<NumericEffectOperator<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<typename V::template ViewVariant<Repository>>(), "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<FactKind T>
void bind_ground_numeric_effect_operator_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundNumericEffectOperator<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<typename V::template ViewVariant<Repository>>(), "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_function_expression_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<FunctionExpression>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<typename V::template ViewVariant<Repository>>(), "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_conjunctive_condition_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<ConjunctiveCondition>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const VariableViewList&,
                                 const LiteralViewList<StaticTag>&,
                                 const LiteralViewList<FluentTag>&,
                                 const LiteralViewList<DerivedTag>&,
                                 const LiftedBooleanOperatorViewList&>(),
                        "variables"_a,
                        "static_literals"_a,
                        "fluent_literals"_a,
                        "derived_literals"_a,
                        "numeric_constraints"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_conjunctive_effect_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<ConjunctiveEffect>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const LiteralViewList<FluentTag>&,
                                 const NumericEffectOperatorViewList<FluentTag>&,
                                 const std::optional<NumericEffectOperatorView<AuxiliaryTag>>&>(),
                        "fluent_literals"_a,
                        "fluent_numeric_effects"_a,
                        "auxiliary_numeric_effect"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_conditional_effect_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<ConditionalEffect>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const VariableViewList&, ConjunctiveConditionView, ConjunctiveEffectView>(), "variables"_a, "condition"_a, "effect"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_action_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Action>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const std::string&, ygg::uint_t, const VariableViewList&, ConjunctiveConditionView, const ConditionalEffectViewList&>(),
                        "name"_a,
                        "original_arity"_a,
                        "variables"_a,
                        "condition"_a,
                        "effects"_a)
                   .def(nb::init<const std::string&,
                                 const std::string&,
                                 ygg::uint_t,
                                 const VariableViewList&,
                                 ConjunctiveConditionView,
                                 const ConditionalEffectViewList&>(),
                        "name"_a,
                        "original_name"_a,
                        "original_arity"_a,
                        "variables"_a,
                        "condition"_a,
                        "effects"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_axiom_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Axiom>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const VariableViewList&, ConjunctiveConditionView, AtomView<DerivedTag>>(), "variables"_a, "body"_a, "head"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_function_expression_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundFunctionExpression>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<typename V::template ViewVariant<Repository>>(), "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_conjunctive_condition_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundConjunctiveCondition>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const GroundLiteralViewList<StaticTag>&,
                                 const GroundLiteralViewList<DerivedTag>&,
                                 const FDRFactViewList<FluentTag>&,
                                 const FDRFactViewList<FluentTag>&,
                                 const GroundBooleanOperatorViewList&>(),
                        "static_literals"_a,
                        "derived_literals"_a,
                        "positive_facts"_a,
                        "negative_facts"_a,
                        "numeric_constraints"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_conjunctive_effect_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundConjunctiveEffect>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const FDRFactViewList<FluentTag>&,
                                 const FDRFactViewList<FluentTag>&,
                                 const GroundNumericEffectOperatorViewList<FluentTag>&,
                                 const std::optional<GroundNumericEffectOperatorView<AuxiliaryTag>>&>(),
                        "add_facts"_a,
                        "del_facts"_a,
                        "fluent_numeric_effects"_a,
                        "auxiliary_numeric_effect"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_conditional_effect_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundConditionalEffect>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<GroundConjunctiveConditionView, GroundConjunctiveEffectView>(), "condition"_a, "effect"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_action_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundAction>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<ActionBindingView, GroundConjunctiveConditionView, const GroundConditionalEffectViewList&>(),
                        "binding"_a,
                        "condition"_a,
                        "effects"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_axiom_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<GroundAxiom>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<AxiomBindingView, GroundConjunctiveConditionView, GroundAtomView<DerivedTag>>(), "binding"_a, "body"_a, "head"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_metric_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Metric>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<typename V::ObjectiveVariant, GroundFunctionExpressionView>(), "objective_kind"_a, "fexpr"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_domain_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Domain>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const std::string&,
                                 const PredicateViewList<StaticTag>&,
                                 const PredicateViewList<FluentTag>&,
                                 const PredicateViewList<DerivedTag>&,
                                 const FunctionViewList<StaticTag>&,
                                 const FunctionViewList<FluentTag>&,
                                 const std::optional<FunctionView<AuxiliaryTag>>&,
                                 const ObjectViewList&,
                                 const ActionViewList&,
                                 const AxiomViewList&>(),
                        "name"_a,
                        "static_predicates"_a,
                        "fluent_predicates"_a,
                        "derived_predicates"_a,
                        "static_functions"_a,
                        "fluent_functions"_a,
                        "auxiliary_function"_a,
                        "constants"_a,
                        "actions"_a,
                        "axioms"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_lifted_task_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<Task>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const std::string&,
                                 DomainView,
                                 const PredicateViewList<DerivedTag>&,
                                 const ObjectViewList&,
                                 const GroundAtomViewList<StaticTag>&,
                                 const GroundAtomViewList<FluentTag>&,
                                 const GroundFunctionTermValueViewList<StaticTag>&,
                                 const GroundFunctionTermValueViewList<FluentTag>&,
                                 const std::optional<GroundFunctionTermValueView<AuxiliaryTag>>&,
                                 GroundConjunctiveConditionView,
                                 const std::optional<MetricView>&,
                                 const AxiomViewList&>(),
                        "name"_a,
                        "domain"_a,
                        "derived_predicates"_a,
                        "objects"_a,
                        "static_atoms"_a,
                        "fluent_atoms"_a,
                        "static_fterm_values"_a,
                        "fluent_fterm_values"_a,
                        "auxiliary_fterm_value"_a,
                        "goal"_a,
                        "metric"_a,
                        "axioms"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

void bind_ground_task_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<FDRTask>;

    auto cls = nb::class_<V>(m, name.c_str())  //

                   .def(nb::init<const std::string&,
                                 DomainView,
                                 const PredicateViewList<DerivedTag>&,
                                 const ObjectViewList&,
                                 const GroundAtomViewList<StaticTag>&,
                                 const GroundAtomViewList<FluentTag>&,
                                 const GroundAtomViewList<DerivedTag>&,
                                 const GroundFunctionTermValueViewList<StaticTag>&,
                                 const GroundFunctionTermValueViewList<FluentTag>&,
                                 const std::optional<GroundFunctionTermValueView<AuxiliaryTag>>&,
                                 const std::optional<MetricView>&,
                                 const AxiomViewList&,
                                 const FDRVariableViewList<FluentTag>&,
                                 const FDRFactViewList<FluentTag>&,
                                 GroundConjunctiveConditionView,
                                 const GroundActionViewList&,
                                 const GroundAxiomViewList&>(),
                        "name"_a,
                        "domain"_a,
                        "derived_predicates"_a,
                        "objects"_a,
                        "static_atoms"_a,
                        "fluent_atoms"_a,
                        "derived_atoms"_a,
                        "static_fterm_values"_a,
                        "fluent_fterm_values"_a,
                        "auxiliary_fterm_value"_a,
                        "metric"_a,
                        "axioms"_a,
                        "fluent_variables"_a,
                        "fluent_facts"_a,
                        "goal"_a,
                        "ground_actions"_a,
                        "ground_axioms"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<OpKind Op, typename T>
void bind_unary_operator_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<UnaryOperator<Op, T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<ygg::View<T, Repository>>(), "arg"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<OpKind Op, typename T>
void bind_binary_operator_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<BinaryOperator<Op, T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<ygg::View<T, Repository>, ygg::View<T, Repository>>(), "lhs"_a, "rhs"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<OpKind Op, typename T>
void bind_multi_operator_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<MultiOperator<Op, T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<const std::vector<ygg::View<T, Repository>>&>(), "args"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<typename T>
void bind_arithmetic_operator_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<ArithmeticOperator<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<typename V::template ViewVariant<Repository>>(), "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

template<typename T>
void bind_boolean_operator_data(nb::module_& m, const std::string& name)
{
    using V = ygg::Data<BooleanOperator<T>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def(nb::init<typename V::template ViewVariant<Repository>>(), "value"_a);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}
}

/**
 * bind_datas
 */

void bind_datas(nb::module_& m)
{
    bind_relation_binding_data<Predicate<DerivedTag>>(m, "DerivedPredicateBindingData");
    bind_relation_binding_data<Action>(m, "ActionBindingData");
    bind_relation_binding_data<Axiom>(m, "AxiomBindingData");

    bind_predicate_data<DerivedTag>(m, "DerivedPredicateData");

    bind_atom_data<StaticTag>(m, "StaticAtomData");
    bind_atom_data<FluentTag>(m, "FluentAtomData");
    bind_atom_data<DerivedTag>(m, "DerivedAtomData");

    bind_ground_atom_data<StaticTag>(m, "StaticGroundAtomData");
    bind_ground_atom_data<FluentTag>(m, "FluentGroundAtomData");
    bind_ground_atom_data<DerivedTag>(m, "DerivedGroundAtomData");

    bind_literal_data<StaticTag>(m, "StaticLiteralData");
    bind_literal_data<FluentTag>(m, "FluentLiteralData");
    bind_literal_data<DerivedTag>(m, "DerivedLiteralData");

    bind_ground_literal_data<StaticTag>(m, "StaticGroundLiteralData");
    bind_ground_literal_data<FluentTag>(m, "FluentGroundLiteralData");
    bind_ground_literal_data<DerivedTag>(m, "DerivedGroundLiteralData");

    bind_fdr_variable_data<FluentTag>(m, "FluentFDRVariableData");
    bind_fdr_fact_data<FluentTag>(m, "FluentFDRFactData");

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

    bind_function_expression_data(m, "FunctionExpressionData");
    bind_conjunctive_condition_data(m, "ConjunctiveConditionData");

    bind_numeric_effect_data<Assign, FluentTag>(m, "FluentNumericEffectAssignData");
    bind_numeric_effect_data<Increase, FluentTag>(m, "FluentNumericEffectIncreaseData");
    bind_numeric_effect_data<Decrease, FluentTag>(m, "FluentNumericEffectDecreaseData");
    bind_numeric_effect_data<ScaleUp, FluentTag>(m, "FluentNumericEffectScaleUpData");
    bind_numeric_effect_data<ScaleDown, FluentTag>(m, "FluentNumericEffectScaleDownData");
    bind_numeric_effect_data<Increase, AuxiliaryTag>(m, "AuxiliaryNumericEffectIncreaseData");

    bind_numeric_effect_operator_data<FluentTag>(m, "FluentNumericEffectOperatorData");
    bind_numeric_effect_operator_data<AuxiliaryTag>(m, "AuxiliaryNumericEffectOperatorData");

    bind_conjunctive_effect_data(m, "ConjunctiveEffectData");
    bind_conditional_effect_data(m, "ConditionalEffectData");
    bind_action_data(m, "ActionData");
    bind_axiom_data(m, "AxiomData");

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

    bind_ground_function_expression_data(m, "GroundFunctionExpressionData");

    bind_ground_conjunctive_condition_data(m, "GroundConjunctiveConditionData");

    bind_ground_numeric_effect_data<Assign, FluentTag>(m, "FluentGroundNumericEffectAssignData");
    bind_ground_numeric_effect_data<Increase, FluentTag>(m, "FluentGroundNumericEffectIncreaseData");
    bind_ground_numeric_effect_data<Decrease, FluentTag>(m, "FluentGroundNumericEffectDecreaseData");
    bind_ground_numeric_effect_data<ScaleUp, FluentTag>(m, "FluentGroundNumericEffectScaleUpData");
    bind_ground_numeric_effect_data<ScaleDown, FluentTag>(m, "FluentGroundNumericEffectScaleDownData");
    bind_ground_numeric_effect_data<Increase, AuxiliaryTag>(m, "AuxiliaryGroundNumericEffectIncreaseData");

    bind_ground_numeric_effect_operator_data<FluentTag>(m, "FluentGroundNumericEffectOperatorData");
    bind_ground_numeric_effect_operator_data<AuxiliaryTag>(m, "AuxiliaryGroundNumericEffectOperatorData");

    bind_ground_conjunctive_effect_data(m, "GroundConjunctiveEffectData");
    bind_ground_conditional_effect_data(m, "GroundConditionalEffectData");
    bind_ground_action_data(m, "GroundActionData");
    bind_ground_axiom_data(m, "GroundAxiomData");

    bind_metric_data(m, "MetricData");
    bind_domain_data(m, "DomainData");
    bind_lifted_task_data(m, "LiftedTaskData");
    bind_ground_task_data(m, "GroundTaskData");
}

}
