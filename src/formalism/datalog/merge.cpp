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

#include "tyr/formalism/datalog/merge.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::datalog
{
template std::pair<PredicateView<StaticTag>, bool> merge_d2d(PredicateView<StaticTag> element, MergeContext& context);
template std::pair<PredicateView<FluentTag>, bool> merge_d2d(PredicateView<FluentTag> element, MergeContext& context);

template std::pair<AtomView<StaticTag>, bool> merge_d2d(AtomView<StaticTag> element, MergeContext& context);
template std::pair<AtomView<FluentTag>, bool> merge_d2d(AtomView<FluentTag> element, MergeContext& context);

template std::pair<PredicateBindingView<StaticTag>, bool> merge_d2d(PredicateBindingView<StaticTag> element, MergeContext& context);
template std::pair<PredicateBindingView<FluentTag>, bool> merge_d2d(PredicateBindingView<FluentTag> element, MergeContext& context);

template std::pair<GroundAtomView<StaticTag>, bool> merge_d2d(GroundAtomView<StaticTag> element, MergeContext& context);
template std::pair<GroundAtomView<FluentTag>, bool> merge_d2d(GroundAtomView<FluentTag> element, MergeContext& context);

template std::pair<LiteralView<StaticTag>, bool> merge_d2d(LiteralView<StaticTag> element, MergeContext& context);
template std::pair<LiteralView<FluentTag>, bool> merge_d2d(LiteralView<FluentTag> element, MergeContext& context);

template std::pair<GroundLiteralView<StaticTag>, bool> merge_d2d(GroundLiteralView<StaticTag> element, MergeContext& context);
template std::pair<GroundLiteralView<FluentTag>, bool> merge_d2d(GroundLiteralView<FluentTag> element, MergeContext& context);

template std::pair<FunctionView<StaticTag>, bool> merge_d2d(FunctionView<StaticTag> element, MergeContext& context);
template std::pair<FunctionView<FluentTag>, bool> merge_d2d(FunctionView<FluentTag> element, MergeContext& context);

template std::pair<FunctionTermView<StaticTag>, bool> merge_d2d(FunctionTermView<StaticTag> element, MergeContext& context);
template std::pair<FunctionTermView<FluentTag>, bool> merge_d2d(FunctionTermView<FluentTag> element, MergeContext& context);

template std::pair<FunctionBindingView<StaticTag>, bool> merge_d2d(FunctionBindingView<StaticTag> element, MergeContext& context);
template std::pair<FunctionBindingView<FluentTag>, bool> merge_d2d(FunctionBindingView<FluentTag> element, MergeContext& context);

template std::pair<GroundFunctionTermView<StaticTag>, bool> merge_d2d(GroundFunctionTermView<StaticTag> element, MergeContext& context);
template std::pair<GroundFunctionTermView<FluentTag>, bool> merge_d2d(GroundFunctionTermView<FluentTag> element, MergeContext& context);

template std::pair<GroundFunctionTermValueView<StaticTag>, bool> merge_d2d(GroundFunctionTermValueView<StaticTag> element, MergeContext& context);
template std::pair<GroundFunctionTermValueView<FluentTag>, bool> merge_d2d(GroundFunctionTermValueView<FluentTag> element, MergeContext& context);

template std::pair<UnaryOperatorView<Sub, ygg::Data<FunctionExpression>>, bool> merge_d2d(UnaryOperatorView<Sub, ygg::Data<FunctionExpression>> element,
                                                                                          MergeContext& context);
template std::pair<UnaryOperatorView<Sub, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(UnaryOperatorView<Sub, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

template std::pair<BinaryOperatorView<Eq, ygg::Data<FunctionExpression>>, bool> merge_d2d(BinaryOperatorView<Eq, ygg::Data<FunctionExpression>> element,
                                                                                          MergeContext& context);
template std::pair<BinaryOperatorView<Ne, ygg::Data<FunctionExpression>>, bool> merge_d2d(BinaryOperatorView<Ne, ygg::Data<FunctionExpression>> element,
                                                                                          MergeContext& context);
template std::pair<BinaryOperatorView<Ge, ygg::Data<FunctionExpression>>, bool> merge_d2d(BinaryOperatorView<Ge, ygg::Data<FunctionExpression>> element,
                                                                                          MergeContext& context);
template std::pair<BinaryOperatorView<Gt, ygg::Data<FunctionExpression>>, bool> merge_d2d(BinaryOperatorView<Gt, ygg::Data<FunctionExpression>> element,
                                                                                          MergeContext& context);
template std::pair<BinaryOperatorView<Le, ygg::Data<FunctionExpression>>, bool> merge_d2d(BinaryOperatorView<Le, ygg::Data<FunctionExpression>> element,
                                                                                          MergeContext& context);
template std::pair<BinaryOperatorView<Lt, ygg::Data<FunctionExpression>>, bool> merge_d2d(BinaryOperatorView<Lt, ygg::Data<FunctionExpression>> element,
                                                                                          MergeContext& context);
template std::pair<BinaryOperatorView<Add, ygg::Data<FunctionExpression>>, bool> merge_d2d(BinaryOperatorView<Add, ygg::Data<FunctionExpression>> element,
                                                                                           MergeContext& context);
template std::pair<BinaryOperatorView<Sub, ygg::Data<FunctionExpression>>, bool> merge_d2d(BinaryOperatorView<Sub, ygg::Data<FunctionExpression>> element,
                                                                                           MergeContext& context);
template std::pair<BinaryOperatorView<Mul, ygg::Data<FunctionExpression>>, bool> merge_d2d(BinaryOperatorView<Mul, ygg::Data<FunctionExpression>> element,
                                                                                           MergeContext& context);
template std::pair<BinaryOperatorView<Div, ygg::Data<FunctionExpression>>, bool> merge_d2d(BinaryOperatorView<Div, ygg::Data<FunctionExpression>> element,
                                                                                           MergeContext& context);
template std::pair<BinaryOperatorView<Eq, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<Eq, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
template std::pair<BinaryOperatorView<Ne, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<Ne, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
template std::pair<BinaryOperatorView<Ge, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<Ge, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
template std::pair<BinaryOperatorView<Gt, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<Gt, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
template std::pair<BinaryOperatorView<Le, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<Le, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
template std::pair<BinaryOperatorView<Lt, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<Lt, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
template std::pair<BinaryOperatorView<Add, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<Add, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
template std::pair<BinaryOperatorView<Sub, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<Sub, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
template std::pair<BinaryOperatorView<Mul, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<Mul, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
template std::pair<BinaryOperatorView<Div, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<Div, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

template std::pair<MultiOperatorView<Add, ygg::Data<FunctionExpression>>, bool> merge_d2d(MultiOperatorView<Add, ygg::Data<FunctionExpression>> element,
                                                                                          MergeContext& context);
template std::pair<MultiOperatorView<Mul, ygg::Data<FunctionExpression>>, bool> merge_d2d(MultiOperatorView<Mul, ygg::Data<FunctionExpression>> element,
                                                                                          MergeContext& context);
template std::pair<MultiOperatorView<Add, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(MultiOperatorView<Add, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
template std::pair<MultiOperatorView<Mul, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(MultiOperatorView<Mul, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

template ArithmeticOperatorView<ygg::Data<FunctionExpression>> merge_d2d(ArithmeticOperatorView<ygg::Data<FunctionExpression>> element,
                                                                          MergeContext& context);
template ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> merge_d2d(ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> element,
                                                                                MergeContext& context);

template std::pair<NumericEffectView<Assign, FluentTag>, bool> merge_d2d(NumericEffectView<Assign, FluentTag> element, MergeContext& context);
template std::pair<NumericEffectView<Increase, FluentTag>, bool> merge_d2d(NumericEffectView<Increase, FluentTag> element, MergeContext& context);
template std::pair<NumericEffectView<Decrease, FluentTag>, bool> merge_d2d(NumericEffectView<Decrease, FluentTag> element, MergeContext& context);
template std::pair<NumericEffectView<ScaleUp, FluentTag>, bool> merge_d2d(NumericEffectView<ScaleUp, FluentTag> element, MergeContext& context);
template std::pair<NumericEffectView<ScaleDown, FluentTag>, bool> merge_d2d(NumericEffectView<ScaleDown, FluentTag> element, MergeContext& context);
template NumericEffectOperatorView<FluentTag> merge_d2d(NumericEffectOperatorView<FluentTag> element, MergeContext& context);

template std::pair<RuleView<PredicateTag>, bool> merge_d2d(RuleView<PredicateTag> element, MergeContext& context);
template std::pair<RuleView<FunctionTag>, bool> merge_d2d(RuleView<FunctionTag> element, MergeContext& context);
}

#endif
