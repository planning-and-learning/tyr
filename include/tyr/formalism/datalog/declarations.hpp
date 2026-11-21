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

#ifndef TYR_FORMALISM_DATALOG_DECLARATIONS_HPP_
#define TYR_FORMALISM_DATALOG_DECLARATIONS_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"

#include <boost/mp11.hpp>
#include <cista/containers/variant.h>
#include <cista/containers/vector.h>
#include <concepts>
#include <memory>
#include <string_view>
#include <urd/elements/concepts.hpp>
#include <urd/elements/flat.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <variant>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/builder.hpp>

namespace tyr::formalism::datalog
{

/// One repository holds both formalisms (planning/declarations.hpp): a program is a layer over its task's repository.
using Repository = planning::Repository;
using Builder = planning::Builder;
using RepositoryPtr = planning::RepositoryPtr;

/**
 * Declaration-only view aliases.
 *
 * Include repository.hpp before constructing, storing, or accessing views.
 */
template<TaskKind T, OperatorKind O>
using BinaryOperatorView = urd::index_view<BinaryOperator<T, O>, Repository>;

template<FactKind F>
using PredicateRowView = ygg::RowView<Predicate<F>, Repository>;
template<FactKind F>
using FunctionRowView = ygg::RowView<Function<F>, Repository>;
using RuleRowView = ygg::RowView<Rule<LiftedTag>, Repository>;

template<FactKind F>
using PredicateRowIndex = ygg::RowIndex<Predicate<F>>;
template<FactKind F>
using FunctionRowIndex = ygg::RowIndex<Function<F>>;
using RuleRowIndex = ygg::RowIndex<Rule<LiftedTag>>;

template<FactKind F>
using PredicateRowForwardRangeView = ygg::RowsView<Predicate<F>, urd::Store<urd::sequence<urd::index<ygg::Row<Predicate<F>>>>>, Repository>;
template<FactKind F>
using FunctionRowRandomAccessRangeView = ygg::RowsView<Function<F>, urd::Store<urd::sequence<urd::index<ygg::Row<Function<F>>>>>, Repository>;

template<TaskKind T>
using BooleanOperatorView = urd::index_view<BinaryOperator<T, ::tyr::formalism::BooleanOperatorKind>, Repository>;

template<TaskKind T>
using BooleanOperatorListView = urd::resolved_range<urd::flat_vector<urd::index<BinaryOperator<T, ::tyr::formalism::BooleanOperatorKind>>>, Repository>;
template<TaskKind T>
using BooleanOperatorViewList = urd::Store<urd::sequence<BooleanOperatorView<T>>>;

template<FactKind F>
using FunctionView = urd::index_view<Function<F>, Repository>;

template<FactKind F>
using FunctionListView = urd::resolved_range<urd::flat_vector<urd::index<Function<F>>>, Repository>;
template<FactKind F>
using FunctionViewList = urd::Store<urd::sequence<FunctionView<F>>>;

template<TaskKind T, OperatorKind O>
using MultiOperatorView = urd::index_view<MultiOperator<T, O>, Repository>;

using ObjectView = urd::index_view<Object, Repository>;

using ObjectViewList = urd::Store<urd::sequence<ObjectView>>;

template<FactKind F>
using PredicateView = urd::index_view<Predicate<F>, Repository>;

template<FactKind F>
using PredicateListView = urd::resolved_range<urd::flat_vector<urd::index<Predicate<F>>>, Repository>;
template<FactKind F>
using PredicateViewList = urd::Store<urd::sequence<PredicateView<F>>>;

/// A term read through the repository: its object's view, or the parameter (formalism/declarations.hpp).
using TermView = std::variant<ObjectView, ParameterIndex>;

using TermListView = urd::resolved_range<TermList, Repository>;
using TermViewList = urd::Store<urd::sequence<TermView>>;

template<TaskKind T, OperatorKind O>
using UnaryOperatorView = urd::index_view<UnaryOperator<T, O>, Repository>;

using VariableView = urd::index_view<Variable, Repository>;
using ParametersView = urd::index_view<Parameters, Repository>;

using VariableViewList = urd::Store<urd::sequence<VariableView>>;

template<TaskKind T, FactKind F>
using AtomListView = urd::resolved_range<urd::flat_vector<urd::index<Atom<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using AtomView = urd::index_view<Atom<T, F>, Repository>;
template<TaskKind T, FactKind F>
using AtomViewList = urd::Store<urd::sequence<AtomView<T, F>>>;
using RuleBodyView = urd::index_view<RuleBody<GroundTag>, Repository>;
/// The body of a rule of kind T: a lifted rule's conjunctive condition, a ground rule's RuleBody.
template<TaskKind T>
using ConjunctiveConditionView = std::conditional_t<std::same_as<T, GroundTag>, RuleBodyView, urd::index_view<ConjunctiveCondition<LiftedTag>, Repository>>;
template<TaskKind T, FactKind F>
using FunctionTermListView = urd::resolved_range<urd::flat_vector<urd::index<FunctionTerm<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueListView = urd::resolved_range<urd::flat_vector<urd::index<FunctionTermValue<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueView = urd::index_view<FunctionTermValue<T, F>, Repository>;
template<TaskKind T, FactKind F>
using FunctionTermValueViewList = urd::Store<urd::sequence<FunctionTermValueView<T, F>>>;
template<TaskKind T, FactKind F>
using FunctionTermView = urd::index_view<FunctionTerm<T, F>, Repository>;
template<TaskKind T, FactKind F>
using LiteralListView = urd::resolved_range<urd::flat_vector<urd::index<Literal<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using LiteralView = urd::index_view<Literal<T, F>, Repository>;
template<TaskKind T, FactKind F>
using LiteralViewList = urd::Store<urd::sequence<LiteralView<T, F>>>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorListView = urd::resolved_range<urd::flat_vector<urd::index<NumericEffect<T, F>>>, Repository>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorView = urd::index_view<NumericEffect<T, F>, Repository>;
template<TaskKind T, FactKind F>
using NumericEffectOperatorViewList = urd::Store<urd::sequence<NumericEffectOperatorView<T, F>>>;
template<TaskKind T, FactKind F>
using NumericEffectView = urd::index_view<NumericEffect<T, F>, Repository>;
template<TaskKind T>
using ProgramView = urd::index_view<Program<T>, Repository>;
template<TaskKind T>
using RuleView = urd::index_view<Rule<T>, Repository>;
template<TaskKind T>
using RuleViewList = urd::Store<urd::sequence<RuleView<T>>>;

/// A function expression read through the repository: the number, or the view of the record, it holds.
template<TaskKind T>
using FunctionExpressionView = std::conditional_t<std::same_as<T, GroundTag>,
                                                  std::variant<ygg::float_t,
                                                               UnaryOperatorView<T, ArithmeticOperatorKind>,
                                                               BinaryOperatorView<T, ArithmeticOperatorKind>,
                                                               MultiOperatorView<T, ArithmeticOperatorKind>,
                                                               FunctionTermView<T, StaticTag>,
                                                               FunctionTermView<T, FluentTag>,
                                                               FunctionTermView<T, AuxiliaryTag>>,
                                                  std::variant<ygg::float_t,
                                                               UnaryOperatorView<T, ArithmeticOperatorKind>,
                                                               BinaryOperatorView<T, ArithmeticOperatorKind>,
                                                               MultiOperatorView<T, ArithmeticOperatorKind>,
                                                               FunctionTermView<T, StaticTag>,
                                                               FunctionTermView<T, FluentTag>>>;
class VariableDependencyGraph;

}

#endif
