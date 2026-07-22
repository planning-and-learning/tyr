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

#include "indices.hpp"

#include <tyr/tyr.hpp>
#include <yggdrasil/python/bindings.hpp>

namespace tyr::formalism::planning
{
using ygg::bind_index;

namespace
{
template<typename Tag>
void bind_relation_binding_index(nb::module_& m, const std::string& name)
{
    using V = ygg::Index<RelationBinding<Tag>>;

    auto cls = nb::class_<V>(m, name.c_str())  //
                   .def_ro("relation_index", &V::relation)
                   .def_ro("row_index", &V::row);
    ygg::add_print(cls);
    ygg::add_comparison(cls);
    ygg::add_hash(cls);
}

}

/**
 * bind_indices
 */

void bind_indices(nb::module_& m)
{
    bind_index<ygg::Index<Row>>(m, "RowIndex");
    bind_index<ygg::Index<Object>>(m, "ObjectIndex");
    bind_index<ygg::Index<Variable>>(m, "VariableIndex");

    bind_relation_binding_index<Predicate<StaticTag>>(m, "StaticPredicateBindingIndex");
    bind_relation_binding_index<Predicate<FluentTag>>(m, "FluentPredicateBindingIndex");
    bind_relation_binding_index<Predicate<DerivedTag>>(m, "DerivedPredicateBindingIndex");
    bind_relation_binding_index<Function<StaticTag>>(m, "StaticFunctionBindingIndex");
    bind_relation_binding_index<Function<FluentTag>>(m, "FluentFunctionBindingIndex");
    bind_relation_binding_index<Function<AuxiliaryTag>>(m, "AuxiliaryFunctionBindingIndex");
    bind_relation_binding_index<Action>(m, "ActionBindingIndex");
    bind_relation_binding_index<Axiom>(m, "AxiomBindingIndex");

    bind_index<ygg::Index<Predicate<StaticTag>>>(m, "StaticPredicateIndex");
    bind_index<ygg::Index<Predicate<FluentTag>>>(m, "FluentPredicateIndex");
    bind_index<ygg::Index<Predicate<DerivedTag>>>(m, "DerivedPredicateIndex");

    bind_index<ygg::Index<Atom<StaticTag>>>(m, "StaticAtomIndex");
    bind_index<ygg::Index<Atom<FluentTag>>>(m, "FluentAtomIndex");
    bind_index<ygg::Index<Atom<DerivedTag>>>(m, "DerivedAtomIndex");

    bind_index<ygg::Index<GroundAtom<StaticTag>>>(m, "StaticGroundAtomIndex");
    bind_index<ygg::Index<GroundAtom<FluentTag>>>(m, "FluentGroundAtomIndex");
    bind_index<ygg::Index<GroundAtom<DerivedTag>>>(m, "DerivedGroundAtomIndex");

    bind_index<ygg::Index<Literal<StaticTag>>>(m, "StaticLiteralIndex");
    bind_index<ygg::Index<Literal<FluentTag>>>(m, "FluentLiteralIndex");
    bind_index<ygg::Index<Literal<DerivedTag>>>(m, "DerivedLiteralIndex");

    bind_index<ygg::Index<GroundLiteral<StaticTag>>>(m, "StaticGroundLiteralIndex");
    bind_index<ygg::Index<GroundLiteral<FluentTag>>>(m, "FluentGroundLiteralIndex");
    bind_index<ygg::Index<GroundLiteral<DerivedTag>>>(m, "DerivedGroundLiteralIndex");

    bind_index<ygg::Index<FDRVariable<FluentTag>>>(m, "FluentFDRVariableIndex");

    bind_index<ygg::Index<Function<StaticTag>>>(m, "StaticFunctionIndex");
    bind_index<ygg::Index<Function<FluentTag>>>(m, "FluentFunctionIndex");
    bind_index<ygg::Index<Function<AuxiliaryTag>>>(m, "AuxiliaryFunctionIndex");

    bind_index<ygg::Index<FunctionTerm<StaticTag>>>(m, "StaticFunctionTermIndex");
    bind_index<ygg::Index<FunctionTerm<FluentTag>>>(m, "FluentFunctionTermIndex");
    bind_index<ygg::Index<FunctionTerm<AuxiliaryTag>>>(m, "AuxiliaryFunctionTermIndex");

    bind_index<ygg::Index<GroundFunctionTerm<StaticTag>>>(m, "StaticGroundFunctionTermIndex");
    bind_index<ygg::Index<GroundFunctionTerm<FluentTag>>>(m, "FluentGroundFunctionTermIndex");
    bind_index<ygg::Index<GroundFunctionTerm<AuxiliaryTag>>>(m, "AuxiliaryGroundFunctionTermIndex");

    bind_index<ygg::Index<GroundFunctionTermValue<StaticTag>>>(m, "StaticGroundFunctionTermValueIndex");
    bind_index<ygg::Index<GroundFunctionTermValue<FluentTag>>>(m, "FluentGroundFunctionTermValueIndex");
    bind_index<ygg::Index<GroundFunctionTermValue<AuxiliaryTag>>>(m, "AuxiliaryGroundFunctionTermValueIndex");

    bind_index<ygg::Index<UnaryOperator<Sub, ygg::Data<FunctionExpression>>>>(m, "UnaryOperatorSubIndex");

    bind_index<ygg::Index<BinaryOperator<Add, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorAddIndex");
    bind_index<ygg::Index<BinaryOperator<Sub, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorSubIndex");
    bind_index<ygg::Index<BinaryOperator<Mul, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorMulIndex");
    bind_index<ygg::Index<BinaryOperator<Div, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorDivIndex");
    bind_index<ygg::Index<BinaryOperator<Eq, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorEqIndex");
    bind_index<ygg::Index<BinaryOperator<Ne, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorNeIndex");
    bind_index<ygg::Index<BinaryOperator<Le, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorLeIndex");
    bind_index<ygg::Index<BinaryOperator<Lt, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorLtIndex");
    bind_index<ygg::Index<BinaryOperator<Ge, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorGeIndex");
    bind_index<ygg::Index<BinaryOperator<Gt, ygg::Data<FunctionExpression>>>>(m, "BinaryOperatorGtIndex");

    bind_index<ygg::Index<MultiOperator<Add, ygg::Data<FunctionExpression>>>>(m, "MultiOperatorAddIndex");
    bind_index<ygg::Index<MultiOperator<Mul, ygg::Data<FunctionExpression>>>>(m, "MultiOperatorMulIndex");

    bind_index<ygg::Index<ConjunctiveCondition>>(m, "ConjunctiveConditionIndex");

    bind_index<ygg::Index<NumericEffect<Assign, FluentTag>>>(m, "FluentNumericEffectAssignIndex");
    bind_index<ygg::Index<NumericEffect<Increase, FluentTag>>>(m, "FluentNumericEffectIncreaseIndex");
    bind_index<ygg::Index<NumericEffect<Decrease, FluentTag>>>(m, "FluentNumericEffectDecreaseIndex");
    bind_index<ygg::Index<NumericEffect<ScaleUp, FluentTag>>>(m, "FluentNumericEffectScaleUpIndex");
    bind_index<ygg::Index<NumericEffect<ScaleDown, FluentTag>>>(m, "FluentNumericEffectScaleDownIndex");
    bind_index<ygg::Index<NumericEffect<Increase, AuxiliaryTag>>>(m, "AuxiliaryNumericEffectIncreaseIndex");

    bind_index<ygg::Index<ConjunctiveEffect>>(m, "ConjunctiveEffectIndex");
    bind_index<ygg::Index<ConditionalEffect>>(m, "ConditionalEffectIndex");
    bind_index<ygg::Index<Action>>(m, "ActionIndex");
    bind_index<ygg::Index<Axiom>>(m, "AxiomIndex");

    bind_index<ygg::Index<UnaryOperator<Sub, ygg::Data<GroundFunctionExpression>>>>(m, "GroundUnaryOperatorSubIndex");

    bind_index<ygg::Index<BinaryOperator<Add, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorAddIndex");
    bind_index<ygg::Index<BinaryOperator<Sub, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorSubIndex");
    bind_index<ygg::Index<BinaryOperator<Mul, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorMulIndex");
    bind_index<ygg::Index<BinaryOperator<Div, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorDivIndex");
    bind_index<ygg::Index<BinaryOperator<Eq, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorEqIndex");
    bind_index<ygg::Index<BinaryOperator<Ne, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorNeIndex");
    bind_index<ygg::Index<BinaryOperator<Le, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorLeIndex");
    bind_index<ygg::Index<BinaryOperator<Lt, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorLtIndex");
    bind_index<ygg::Index<BinaryOperator<Ge, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorGeIndex");
    bind_index<ygg::Index<BinaryOperator<Gt, ygg::Data<GroundFunctionExpression>>>>(m, "GroundBinaryOperatorGtIndex");

    bind_index<ygg::Index<MultiOperator<Add, ygg::Data<GroundFunctionExpression>>>>(m, "GroundMultiOperatorAddIndex");
    bind_index<ygg::Index<MultiOperator<Mul, ygg::Data<GroundFunctionExpression>>>>(m, "GroundMultiOperatorMulIndex");

    bind_index<ygg::Index<GroundConjunctiveCondition>>(m, "GroundConjunctiveConditionIndex");

    bind_index<ygg::Index<GroundNumericEffect<Assign, FluentTag>>>(m, "FluentGroundNumericEffectAssignIndex");
    bind_index<ygg::Index<GroundNumericEffect<Increase, FluentTag>>>(m, "FluentGroundNumericEffectIncreaseIndex");
    bind_index<ygg::Index<GroundNumericEffect<Decrease, FluentTag>>>(m, "FluentGroundNumericEffectDecreaseIndex");
    bind_index<ygg::Index<GroundNumericEffect<ScaleUp, FluentTag>>>(m, "FluentGroundNumericEffectScaleUpIndex");
    bind_index<ygg::Index<GroundNumericEffect<ScaleDown, FluentTag>>>(m, "FluentGroundNumericEffectScaleDownIndex");
    bind_index<ygg::Index<GroundNumericEffect<Increase, AuxiliaryTag>>>(m, "AuxiliaryGroundNumericEffectIncreaseIndex");

    bind_index<ygg::Index<GroundConjunctiveEffect>>(m, "GroundConjunctiveEffectIndex");
    bind_index<ygg::Index<GroundConditionalEffect>>(m, "GroundConditionalEffectIndex");
    bind_index<ygg::Index<GroundAction>>(m, "GroundActionIndex");
    bind_index<ygg::Index<GroundAxiom>>(m, "GroundAxiomIndex");
    bind_index<ygg::Index<Metric>>(m, "MetricIndex");
    bind_index<ygg::Index<Domain>>(m, "DomainIndex");
    bind_index<ygg::Index<Task>>(m, "LiftedTaskIndex");
    bind_index<ygg::Index<FDRTask>>(m, "GroundTaskIndex");
}

}
