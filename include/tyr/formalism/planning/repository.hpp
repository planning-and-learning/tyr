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

#ifndef TYR_FORMALISM_PLANNING_REPOSITORY_HPP_
#define TYR_FORMALISM_PLANNING_REPOSITORY_HPP_

#include <yggdrasil/buffer/declarations.hpp>
#include <yggdrasil/buffer/indexed_hash_set.hpp>
#include <yggdrasil/buffer/segmented_buffer.hpp>
#include <yggdrasil/containers/tuple.hpp>
#include <yggdrasil/formalism/relation_repository.hpp>
#include <yggdrasil/formalism/repository.hpp>
#include <yggdrasil/formalism/repository_factory.hpp>
#include <yggdrasil/formalism/symbol_repository.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/datas.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/indices.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <cassert>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace tyr::formalism::planning
{
using ActionView = ygg::View<ygg::Index<Action>, Repository>;
using ActionListView = ygg::View<ygg::IndexList<Action>, Repository>;
using ActionViewList = std::vector<ActionView>;

template<typename T>
using ArithmeticOperatorView = ygg::View<ygg::Data<ArithmeticOperator<T>>, Repository>;
using LiftedArithmeticOperatorView = ygg::View<ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression>>>, Repository>;
using GroundArithmeticOperatorView = ygg::View<ygg::Data<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>, Repository>;

template<typename T>
using ArithmeticOperatorListView = ygg::View<ygg::DataList<ArithmeticOperator<T>>, Repository>;
using LiftedArithmeticOperatorListView = ygg::View<ygg::DataList<ArithmeticOperator<ygg::Data<FunctionExpression>>>, Repository>;
using GroundArithmeticOperatorListView = ygg::View<ygg::DataList<ArithmeticOperator<ygg::Data<GroundFunctionExpression>>>, Repository>;

template<FactKind T>
using AtomView = ygg::View<ygg::Index<Atom<T>>, Repository>;
template<FactKind T>
using AtomListView = ygg::View<ygg::IndexList<Atom<T>>, Repository>;
template<FactKind T>
using AtomViewList = std::vector<AtomView<T>>;

using AxiomView = ygg::View<ygg::Index<Axiom>, Repository>;
using AxiomListView = ygg::View<ygg::IndexList<Axiom>, Repository>;
using AxiomViewList = std::vector<AxiomView>;

template<OpKind Op, typename T>
using BinaryOperatorView = ygg::View<ygg::Index<BinaryOperator<Op, T>>, Repository>;
template<OpKind Op>
using LiftedBinaryOperatorView = ygg::View<ygg::Index<BinaryOperator<Op, ygg::Data<FunctionExpression>>>, Repository>;
template<OpKind Op>
using GroundBinaryOperatorView = ygg::View<ygg::Index<BinaryOperator<Op, ygg::Data<GroundFunctionExpression>>>, Repository>;

template<OpKind Op, typename T>
using BinaryOperatorListView = ygg::View<ygg::IndexList<BinaryOperator<Op, T>>, Repository>;
template<OpKind Op>
using LiftedBinaryOperatorListView = ygg::View<ygg::IndexList<BinaryOperator<Op, ygg::Data<FunctionExpression>>>, Repository>;
template<OpKind Op>
using GroundBinaryOperatorListView = ygg::View<ygg::IndexList<BinaryOperator<Op, ygg::Data<GroundFunctionExpression>>>, Repository>;

template<FactKind T>
using PredicateBindingView = ygg::View<ygg::Index<RelationBinding<Predicate<T>>>, Repository>;
template<FactKind T>
using FunctionBindingView = ygg::View<ygg::Index<RelationBinding<Function<T>>>, Repository>;
using ActionBindingView = ygg::View<ygg::Index<RelationBinding<Action>>, Repository>;
using AxiomBindingView = ygg::View<ygg::Index<RelationBinding<Axiom>>, Repository>;

template<typename T>
using BooleanOperatorView = ygg::View<ygg::Data<BooleanOperator<T>>, Repository>;
using LiftedBooleanOperatorView = ygg::View<ygg::Data<BooleanOperator<ygg::Data<FunctionExpression>>>, Repository>;
using GroundBooleanOperatorView = ygg::View<ygg::Data<BooleanOperator<ygg::Data<GroundFunctionExpression>>>, Repository>;

template<typename T>
using BooleanOperatorListView = ygg::View<ygg::DataList<BooleanOperator<T>>, Repository>;
using LiftedBooleanOperatorListView = ygg::View<ygg::DataList<BooleanOperator<ygg::Data<FunctionExpression>>>, Repository>;
using GroundBooleanOperatorListView = ygg::View<ygg::DataList<BooleanOperator<ygg::Data<GroundFunctionExpression>>>, Repository>;
using LiftedBooleanOperatorViewList = std::vector<ygg::View<ygg::Data<BooleanOperator<ygg::Data<FunctionExpression>>>, Repository>>;
using GroundBooleanOperatorViewList = std::vector<ygg::View<ygg::Data<BooleanOperator<ygg::Data<GroundFunctionExpression>>>, Repository>>;

using ConditionalEffectView = ygg::View<ygg::Index<ConditionalEffect>, Repository>;
using ConditionalEffectListView = ygg::View<ygg::IndexList<ConditionalEffect>, Repository>;
using ConditionalEffectViewList = std::vector<ygg::View<ygg::Index<ConditionalEffect>, Repository>>;

using ConjunctiveConditionView = ygg::View<ygg::Index<ConjunctiveCondition>, Repository>;
using ConjunctiveConditionListView = ygg::View<ygg::IndexList<ConjunctiveCondition>, Repository>;

using ConjunctiveEffectView = ygg::View<ygg::Index<ConjunctiveEffect>, Repository>;
using ConjunctiveEffectListView = ygg::View<ygg::IndexList<ConjunctiveEffect>, Repository>;

using DomainView = ygg::View<ygg::Index<Domain>, Repository>;
using DomainListView = ygg::View<ygg::IndexList<Domain>, Repository>;

template<FactKind T>
using FDRFactView = ygg::View<ygg::Data<FDRFact<T>>, Repository>;
template<FactKind T>
using FDRFactListView = ygg::View<ygg::DataList<FDRFact<T>>, Repository>;
template<FactKind T>
using FDRFactViewList = std::vector<FDRFactView<T>>;

using FDRTaskView = ygg::View<ygg::Index<FDRTask>, Repository>;
using FDRTaskListView = ygg::View<ygg::IndexList<FDRTask>, Repository>;

template<FactKind T>
using FDRVariableView = ygg::View<ygg::Index<FDRVariable<T>>, Repository>;
template<FactKind T>
using FDRVariableListView = ygg::View<ygg::IndexList<FDRVariable<T>>, Repository>;
template<FactKind T>
using FDRVariableViewList = std::vector<FDRVariableView<T>>;

using FunctionExpressionView = ygg::View<ygg::Data<FunctionExpression>, Repository>;
using FunctionExpressionListView = ygg::View<ygg::DataList<FunctionExpression>, Repository>;

template<FactKind T>
using FunctionTermView = ygg::View<ygg::Index<FunctionTerm<T>>, Repository>;
template<FactKind T>
using FunctionTermListView = ygg::View<ygg::IndexList<FunctionTerm<T>>, Repository>;

template<FactKind T>
using FunctionView = ygg::View<ygg::Index<Function<T>>, Repository>;
template<FactKind T>
using FunctionListView = ygg::View<ygg::IndexList<Function<T>>, Repository>;
template<FactKind T>
using FunctionViewList = std::vector<FunctionView<T>>;

using GroundActionView = ygg::View<ygg::Index<GroundAction>, Repository>;
using GroundActionListView = ygg::View<ygg::IndexList<GroundAction>, Repository>;
using GroundActionViewList = std::vector<GroundActionView>;

template<FactKind T>
using GroundAtomView = ygg::View<ygg::Index<GroundAtom<T>>, Repository>;
template<FactKind T>
using GroundAtomListView = ygg::View<ygg::IndexList<GroundAtom<T>>, Repository>;
template<FactKind T>
using GroundAtomViewList = std::vector<GroundAtomView<T>>;

using GroundAxiomView = ygg::View<ygg::Index<GroundAxiom>, Repository>;
using GroundAxiomListView = ygg::View<ygg::IndexList<GroundAxiom>, Repository>;
using GroundAxiomViewList = std::vector<GroundAxiomView>;

using GroundConditionalEffectView = ygg::View<ygg::Index<GroundConditionalEffect>, Repository>;
using GroundConditionalEffectListView = ygg::View<ygg::IndexList<GroundConditionalEffect>, Repository>;
using GroundConditionalEffectViewList = std::vector<GroundConditionalEffectView>;

using GroundConjunctiveConditionView = ygg::View<ygg::Index<GroundConjunctiveCondition>, Repository>;
using GroundConjunctiveConditionListView = ygg::View<ygg::IndexList<GroundConjunctiveCondition>, Repository>;

using GroundConjunctiveEffectView = ygg::View<ygg::Index<GroundConjunctiveEffect>, Repository>;
using GroundConjunctiveEffectListView = ygg::View<ygg::IndexList<GroundConjunctiveEffect>, Repository>;

using GroundFunctionExpressionView = ygg::View<ygg::Data<GroundFunctionExpression>, Repository>;
using GroundFunctionExpressionListView = ygg::View<ygg::DataList<GroundFunctionExpression>, Repository>;

template<FactKind T>
using GroundFunctionTermValueView = ygg::View<ygg::Index<GroundFunctionTermValue<T>>, Repository>;
template<FactKind T>
using GroundFunctionTermValueListView = ygg::View<ygg::IndexList<GroundFunctionTermValue<T>>, Repository>;
template<FactKind T>
using GroundFunctionTermValueViewList = std::vector<GroundFunctionTermValueView<T>>;

template<FactKind T>
using GroundFunctionTermViewValuePair = std::pair<ygg::View<ygg::Index<GroundFunctionTerm<T>>, Repository>, ygg::float_t>;
template<FactKind T>
using GroundFunctionTermViewValuePairList = std::vector<GroundFunctionTermViewValuePair<T>>;

template<FactKind T>
using GroundFunctionTermView = ygg::View<ygg::Index<GroundFunctionTerm<T>>, Repository>;
template<FactKind T>
using GroundFunctionTermListView = ygg::View<ygg::IndexList<GroundFunctionTerm<T>>, Repository>;
template<FactKind T>
using GroundFunctionTermViewList = std::vector<GroundFunctionTermView<T>>;

template<FactKind T>
using GroundLiteralView = ygg::View<ygg::Index<GroundLiteral<T>>, Repository>;
template<FactKind T>
using GroundLiteralListView = ygg::View<ygg::IndexList<GroundLiteral<T>>, Repository>;
template<FactKind T>
using GroundLiteralViewList = std::vector<GroundLiteralView<T>>;

template<FactKind T>
using GroundNumericEffectOperatorView = ygg::View<ygg::Data<GroundNumericEffectOperator<T>>, Repository>;
template<FactKind T>
using GroundNumericEffectOperatorListView = ygg::View<ygg::DataList<GroundNumericEffectOperator<T>>, Repository>;
template<FactKind T>
using GroundNumericEffectOperatorViewList = std::vector<GroundNumericEffectOperatorView<T>>;

template<NumericEffectOpKind Op, FactKind T>
using GroundNumericEffectView = ygg::View<ygg::Index<GroundNumericEffect<Op, T>>, Repository>;
template<NumericEffectOpKind Op, FactKind T>
using GroundNumericEffectListView = ygg::View<ygg::IndexList<GroundNumericEffect<Op, T>>, Repository>;

template<FactKind T>
using LiteralView = ygg::View<ygg::Index<Literal<T>>, Repository>;
template<FactKind T>
using LiteralListView = ygg::View<ygg::IndexList<Literal<T>>, Repository>;
template<FactKind T>
using LiteralViewList = std::vector<LiteralView<T>>;

using MetricView = ygg::View<ygg::Index<Metric>, Repository>;
using MetricListView = ygg::View<ygg::IndexList<Metric>, Repository>;

template<OpKind Op, typename T>
using MultiOperatorView = ygg::View<ygg::Index<MultiOperator<Op, T>>, Repository>;
template<OpKind Op>
using LiftedMultiOperatorView = ygg::View<ygg::Index<MultiOperator<Op, ygg::Data<FunctionExpression>>>, Repository>;
template<OpKind Op>
using GroundMultiOperatorView = ygg::View<ygg::Index<MultiOperator<Op, ygg::Data<GroundFunctionExpression>>>, Repository>;

template<OpKind Op, typename T>
using MultiOperatorListView = ygg::View<ygg::IndexList<MultiOperator<Op, T>>, Repository>;
template<OpKind Op>
using LiftedMultiOperatorListView = ygg::View<ygg::IndexList<MultiOperator<Op, ygg::Data<FunctionExpression>>>, Repository>;
template<OpKind Op>
using GroundMultiOperatorListView = ygg::View<ygg::IndexList<MultiOperator<Op, ygg::Data<GroundFunctionExpression>>>, Repository>;

template<FactKind T>
using NumericEffectOperatorView = ygg::View<ygg::Data<NumericEffectOperator<T>>, Repository>;
template<FactKind T>
using NumericEffectOperatorListView = ygg::View<ygg::DataList<NumericEffectOperator<T>>, Repository>;
template<FactKind T>
using NumericEffectOperatorViewList = std::vector<NumericEffectOperatorView<T>>;

template<NumericEffectOpKind Op, FactKind T>
using NumericEffectView = ygg::View<ygg::Index<NumericEffect<Op, T>>, Repository>;
template<NumericEffectOpKind Op, FactKind T>
using NumericEffectListView = ygg::View<ygg::IndexList<NumericEffect<Op, T>>, Repository>;

using ObjectView = ygg::View<ygg::Index<Object>, Repository>;
using ObjectListView = ygg::View<ygg::IndexList<Object>, Repository>;
using ObjectViewList = std::vector<ObjectView>;

template<FactKind T>
using PredicateView = ygg::View<ygg::Index<Predicate<T>>, Repository>;
template<FactKind T>
using PredicateListView = ygg::View<ygg::IndexList<Predicate<T>>, Repository>;
template<FactKind T>
using PredicateViewList = std::vector<PredicateView<T>>;

using TaskView = ygg::View<ygg::Index<Task>, Repository>;
using TaskListView = ygg::View<ygg::IndexList<Task>, Repository>;

using TermView = ygg::View<ygg::Data<Term>, Repository>;
using TermListView = ygg::View<ygg::DataList<Term>, Repository>;
using TermViewList = std::vector<TermView>;

template<OpKind Op, typename T>
using UnaryOperatorView = ygg::View<ygg::Index<UnaryOperator<Op, T>>, Repository>;
template<OpKind Op>
using LiftedUnaryOperatorView = ygg::View<ygg::Index<UnaryOperator<Op, ygg::Data<FunctionExpression>>>, Repository>;
template<OpKind Op>
using GroundUnaryOperatorView = ygg::View<ygg::Index<UnaryOperator<Op, ygg::Data<GroundFunctionExpression>>>, Repository>;

template<OpKind Op, typename T>
using UnaryOperatorListView = ygg::View<ygg::IndexList<UnaryOperator<Op, T>>, Repository>;
template<OpKind Op>
using LiftedUnaryOperatorListView = ygg::View<ygg::IndexList<UnaryOperator<Op, ygg::Data<FunctionExpression>>>, Repository>;
template<OpKind Op>
using GroundUnaryOperatorListView = ygg::View<ygg::IndexList<UnaryOperator<Op, ygg::Data<GroundFunctionExpression>>>, Repository>;

using VariableView = ygg::View<ygg::Index<Variable>, Repository>;
using VariableListView = ygg::View<ygg::IndexList<Variable>, Repository>;
using VariableViewList = std::vector<VariableView>;

}


#endif
