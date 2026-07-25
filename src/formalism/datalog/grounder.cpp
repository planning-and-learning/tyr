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

#include "tyr/formalism/datalog/grounder.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::datalog
{
template std::pair<FunctionBindingView<StaticTag>, bool> ground(TermListView terms, FunctionView<StaticTag> function, GrounderContext& context);
template std::pair<FunctionBindingView<FluentTag>, bool> ground(TermListView terms, FunctionView<FluentTag> function, GrounderContext& context);

template std::pair<GroundFunctionTermView<StaticTag>, bool> ground(FunctionTermView<StaticTag> element, GrounderContext& context);
template std::pair<GroundFunctionTermView<FluentTag>, bool> ground(FunctionTermView<FluentTag> element, GrounderContext& context);

template std::pair<GroundUnaryOperatorView<Sub>, bool> ground(LiftedUnaryOperatorView<Sub> element, GrounderContext& context);

template std::pair<GroundBinaryOperatorView<Eq>, bool> ground(LiftedBinaryOperatorView<Eq> element, GrounderContext& context);
template std::pair<GroundBinaryOperatorView<Ne>, bool> ground(LiftedBinaryOperatorView<Ne> element, GrounderContext& context);
template std::pair<GroundBinaryOperatorView<Ge>, bool> ground(LiftedBinaryOperatorView<Ge> element, GrounderContext& context);
template std::pair<GroundBinaryOperatorView<Gt>, bool> ground(LiftedBinaryOperatorView<Gt> element, GrounderContext& context);
template std::pair<GroundBinaryOperatorView<Le>, bool> ground(LiftedBinaryOperatorView<Le> element, GrounderContext& context);
template std::pair<GroundBinaryOperatorView<Lt>, bool> ground(LiftedBinaryOperatorView<Lt> element, GrounderContext& context);
template std::pair<GroundBinaryOperatorView<Add>, bool> ground(LiftedBinaryOperatorView<Add> element, GrounderContext& context);
template std::pair<GroundBinaryOperatorView<Sub>, bool> ground(LiftedBinaryOperatorView<Sub> element, GrounderContext& context);
template std::pair<GroundBinaryOperatorView<Mul>, bool> ground(LiftedBinaryOperatorView<Mul> element, GrounderContext& context);
template std::pair<GroundBinaryOperatorView<Div>, bool> ground(LiftedBinaryOperatorView<Div> element, GrounderContext& context);

template std::pair<GroundMultiOperatorView<Add>, bool> ground(LiftedMultiOperatorView<Add> element, GrounderContext& context);
template std::pair<GroundMultiOperatorView<Mul>, bool> ground(LiftedMultiOperatorView<Mul> element, GrounderContext& context);

template std::pair<PredicateBindingView<StaticTag>, bool> ground(TermListView terms, PredicateView<StaticTag> predicate, GrounderContext& context);
template std::pair<PredicateBindingView<FluentTag>, bool> ground(TermListView terms, PredicateView<FluentTag> predicate, GrounderContext& context);

template std::pair<GroundAtomView<StaticTag>, bool> ground(AtomView<StaticTag> element, GrounderContext& context);
template std::pair<GroundAtomView<FluentTag>, bool> ground(AtomView<FluentTag> element, GrounderContext& context);

template std::pair<GroundLiteralView<StaticTag>, bool> ground(LiteralView<StaticTag> element, GrounderContext& context);
template std::pair<GroundLiteralView<FluentTag>, bool> ground(LiteralView<FluentTag> element, GrounderContext& context);

template std::pair<GroundNumericEffectView<Assign, FluentTag>, bool> ground(NumericEffectView<Assign, FluentTag> element, GrounderContext& context);
template std::pair<GroundNumericEffectView<Increase, FluentTag>, bool> ground(NumericEffectView<Increase, FluentTag> element, GrounderContext& context);
template std::pair<GroundNumericEffectView<Decrease, FluentTag>, bool> ground(NumericEffectView<Decrease, FluentTag> element, GrounderContext& context);
template std::pair<GroundNumericEffectView<ScaleUp, FluentTag>, bool> ground(NumericEffectView<ScaleUp, FluentTag> element, GrounderContext& context);
template std::pair<GroundNumericEffectView<ScaleDown, FluentTag>, bool> ground(NumericEffectView<ScaleDown, FluentTag> element, GrounderContext& context);
template ygg::Data<GroundNumericEffectOperator<FluentTag>> ground(NumericEffectOperatorView<FluentTag> element, GrounderContext& context);

template std::pair<PredicateBindingView<StaticTag>, bool> ground_binding(AtomView<StaticTag> element, GrounderContext& context);
template std::pair<PredicateBindingView<FluentTag>, bool> ground_binding(AtomView<FluentTag> element, GrounderContext& context);

template std::pair<FunctionBindingView<StaticTag>, bool> ground_binding(FunctionTermView<StaticTag> element, GrounderContext& context);
template std::pair<FunctionBindingView<FluentTag>, bool> ground_binding(FunctionTermView<FluentTag> element, GrounderContext& context);

template std::optional<FunctionBindingView<StaticTag>> try_ground_binding(::tyr::formalism::datalog::FunctionTermView<StaticTag> element,
                                                                          ::tyr::formalism::datalog::GrounderContext& context);
template std::optional<FunctionBindingView<FluentTag>> try_ground_binding(::tyr::formalism::datalog::FunctionTermView<FluentTag> element,
                                                                          ::tyr::formalism::datalog::GrounderContext& context);

template std::optional<PredicateBindingView<StaticTag>> try_ground_binding(::tyr::formalism::datalog::AtomView<StaticTag> element,
                                                                           ::tyr::formalism::datalog::GrounderContext& context);
template std::optional<PredicateBindingView<FluentTag>> try_ground_binding(::tyr::formalism::datalog::AtomView<FluentTag> element,
                                                                           ::tyr::formalism::datalog::GrounderContext& context);
}

#endif
