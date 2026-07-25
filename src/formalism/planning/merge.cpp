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

#include "tyr/formalism/planning/merge.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{

template std::pair<PredicateBindingView<StaticTag>, bool> merge_p2p(PredicateBindingView<StaticTag> element, MergeContext& context);
template std::pair<PredicateBindingView<FluentTag>, bool> merge_p2p(PredicateBindingView<FluentTag> element, MergeContext& context);
template std::pair<PredicateBindingView<DerivedTag>, bool> merge_p2p(PredicateBindingView<DerivedTag> element, MergeContext& context);

template std::pair<FunctionBindingView<StaticTag>, bool> merge_p2p(FunctionBindingView<StaticTag> element, MergeContext& context);
template std::pair<FunctionBindingView<FluentTag>, bool> merge_p2p(FunctionBindingView<FluentTag> element, MergeContext& context);
template std::pair<FunctionBindingView<AuxiliaryTag>, bool> merge_p2p(FunctionBindingView<AuxiliaryTag> element, MergeContext& context);

template std::pair<PredicateView<StaticTag>, bool> merge_p2p(PredicateView<StaticTag> element, MergeContext& context);
template std::pair<PredicateView<FluentTag>, bool> merge_p2p(PredicateView<FluentTag> element, MergeContext& context);
template std::pair<PredicateView<DerivedTag>, bool> merge_p2p(PredicateView<DerivedTag> element, MergeContext& context);

template std::pair<AtomView<StaticTag>, bool> merge_p2p(AtomView<StaticTag> element, MergeContext& context);
template std::pair<AtomView<FluentTag>, bool> merge_p2p(AtomView<FluentTag> element, MergeContext& context);
template std::pair<AtomView<DerivedTag>, bool> merge_p2p(AtomView<DerivedTag> element, MergeContext& context);

template std::pair<GroundAtomView<StaticTag>, bool> merge_p2p(GroundAtomView<StaticTag> element, MergeContext& context);
template std::pair<GroundAtomView<FluentTag>, bool> merge_p2p(GroundAtomView<FluentTag> element, MergeContext& context);
template std::pair<GroundAtomView<DerivedTag>, bool> merge_p2p(GroundAtomView<DerivedTag> element, MergeContext& context);

template std::pair<LiteralView<StaticTag>, bool> merge_p2p(LiteralView<StaticTag> element, MergeContext& context);
template std::pair<LiteralView<FluentTag>, bool> merge_p2p(LiteralView<FluentTag> element, MergeContext& context);
template std::pair<LiteralView<DerivedTag>, bool> merge_p2p(LiteralView<DerivedTag> element, MergeContext& context);

template std::pair<GroundLiteralView<StaticTag>, bool> merge_p2p(GroundLiteralView<StaticTag> element, MergeContext& context);
template std::pair<GroundLiteralView<FluentTag>, bool> merge_p2p(GroundLiteralView<FluentTag> element, MergeContext& context);
template std::pair<GroundLiteralView<DerivedTag>, bool> merge_p2p(GroundLiteralView<DerivedTag> element, MergeContext& context);

template std::pair<FunctionView<StaticTag>, bool> merge_p2p(FunctionView<StaticTag> element, MergeContext& context);
template std::pair<FunctionView<FluentTag>, bool> merge_p2p(FunctionView<FluentTag> element, MergeContext& context);
template std::pair<FunctionView<AuxiliaryTag>, bool> merge_p2p(FunctionView<AuxiliaryTag> element, MergeContext& context);

template std::pair<FunctionTermView<StaticTag>, bool> merge_p2p(FunctionTermView<StaticTag> element, MergeContext& context);
template std::pair<FunctionTermView<FluentTag>, bool> merge_p2p(FunctionTermView<FluentTag> element, MergeContext& context);
template std::pair<FunctionTermView<AuxiliaryTag>, bool> merge_p2p(FunctionTermView<AuxiliaryTag> element, MergeContext& context);

template std::pair<GroundFunctionTermView<StaticTag>, bool> merge_p2p(GroundFunctionTermView<StaticTag> element, MergeContext& context);
template std::pair<GroundFunctionTermView<FluentTag>, bool> merge_p2p(GroundFunctionTermView<FluentTag> element, MergeContext& context);
template std::pair<GroundFunctionTermView<AuxiliaryTag>, bool> merge_p2p(GroundFunctionTermView<AuxiliaryTag> element, MergeContext& context);

template std::pair<GroundFunctionTermValueView<StaticTag>, bool> merge_p2p(GroundFunctionTermValueView<StaticTag> element, MergeContext& context);
template std::pair<GroundFunctionTermValueView<FluentTag>, bool> merge_p2p(GroundFunctionTermValueView<FluentTag> element, MergeContext& context);
template std::pair<GroundFunctionTermValueView<AuxiliaryTag>, bool> merge_p2p(GroundFunctionTermValueView<AuxiliaryTag> element, MergeContext& context);

template std::pair<UnaryOperatorView<Sub, ygg::Data<FunctionExpression>>, bool> merge_p2p(UnaryOperatorView<Sub, ygg::Data<FunctionExpression>> element,
                                                                                     MergeContext& context);
template std::pair<UnaryOperatorView<Sub, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(UnaryOperatorView<Sub, ygg::Data<GroundFunctionExpression>> element,
                                                                                           MergeContext& context);

template std::pair<BinaryOperatorView<Eq, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Eq, ygg::Data<FunctionExpression>> element,
                                                                                     MergeContext& context);
template std::pair<BinaryOperatorView<Ne, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Ne, ygg::Data<FunctionExpression>> element,
                                                                                     MergeContext& context);
template std::pair<BinaryOperatorView<Ge, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Ge, ygg::Data<FunctionExpression>> element,
                                                                                     MergeContext& context);
template std::pair<BinaryOperatorView<Gt, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Gt, ygg::Data<FunctionExpression>> element,
                                                                                     MergeContext& context);
template std::pair<BinaryOperatorView<Le, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Le, ygg::Data<FunctionExpression>> element,
                                                                                     MergeContext& context);
template std::pair<BinaryOperatorView<Lt, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Lt, ygg::Data<FunctionExpression>> element,
                                                                                     MergeContext& context);
template std::pair<BinaryOperatorView<Add, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Add, ygg::Data<FunctionExpression>> element,
                                                                                      MergeContext& context);
template std::pair<BinaryOperatorView<Sub, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Sub, ygg::Data<FunctionExpression>> element,
                                                                                      MergeContext& context);
template std::pair<BinaryOperatorView<Mul, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Mul, ygg::Data<FunctionExpression>> element,
                                                                                      MergeContext& context);
template std::pair<BinaryOperatorView<Div, ygg::Data<FunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Div, ygg::Data<FunctionExpression>> element,
                                                                                      MergeContext& context);
template std::pair<BinaryOperatorView<Eq, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Eq, ygg::Data<GroundFunctionExpression>> element,
                                                                                           MergeContext& context);
template std::pair<BinaryOperatorView<Ne, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Ne, ygg::Data<GroundFunctionExpression>> element,
                                                                                           MergeContext& context);
template std::pair<BinaryOperatorView<Ge, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Ge, ygg::Data<GroundFunctionExpression>> element,
                                                                                           MergeContext& context);
template std::pair<BinaryOperatorView<Gt, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Gt, ygg::Data<GroundFunctionExpression>> element,
                                                                                           MergeContext& context);
template std::pair<BinaryOperatorView<Le, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Le, ygg::Data<GroundFunctionExpression>> element,
                                                                                           MergeContext& context);
template std::pair<BinaryOperatorView<Lt, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Lt, ygg::Data<GroundFunctionExpression>> element,
                                                                                           MergeContext& context);
template std::pair<BinaryOperatorView<Add, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Add, ygg::Data<GroundFunctionExpression>> element,
                                                                                            MergeContext& context);
template std::pair<BinaryOperatorView<Sub, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Sub, ygg::Data<GroundFunctionExpression>> element,
                                                                                            MergeContext& context);
template std::pair<BinaryOperatorView<Mul, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Mul, ygg::Data<GroundFunctionExpression>> element,
                                                                                            MergeContext& context);
template std::pair<BinaryOperatorView<Div, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(BinaryOperatorView<Div, ygg::Data<GroundFunctionExpression>> element,
                                                                                            MergeContext& context);

template std::pair<MultiOperatorView<Add, ygg::Data<FunctionExpression>>, bool> merge_p2p(MultiOperatorView<Add, ygg::Data<FunctionExpression>> element,
                                                                                     MergeContext& context);
template std::pair<MultiOperatorView<Mul, ygg::Data<FunctionExpression>>, bool> merge_p2p(MultiOperatorView<Mul, ygg::Data<FunctionExpression>> element,
                                                                                     MergeContext& context);
template std::pair<MultiOperatorView<Add, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(MultiOperatorView<Add, ygg::Data<GroundFunctionExpression>> element,
                                                                                           MergeContext& context);
template std::pair<MultiOperatorView<Mul, ygg::Data<GroundFunctionExpression>>, bool> merge_p2p(MultiOperatorView<Mul, ygg::Data<GroundFunctionExpression>> element,
                                                                                           MergeContext& context);

template ArithmeticOperatorView<ygg::Data<FunctionExpression>> merge_p2p(ArithmeticOperatorView<ygg::Data<FunctionExpression>> element, MergeContext& context);
template ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> merge_p2p(ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> element,
                                                                                MergeContext& context);

template BooleanOperatorView<ygg::Data<FunctionExpression>> merge_p2p(BooleanOperatorView<ygg::Data<FunctionExpression>> element, MergeContext& context);
template BooleanOperatorView<ygg::Data<GroundFunctionExpression>> merge_p2p(BooleanOperatorView<ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

template std::pair<NumericEffectView<Assign, FluentTag>, bool> merge_p2p(NumericEffectView<Assign, FluentTag> element, MergeContext& context);
template std::pair<NumericEffectView<Increase, FluentTag>, bool> merge_p2p(NumericEffectView<Increase, FluentTag> element, MergeContext& context);
template std::pair<NumericEffectView<Decrease, FluentTag>, bool> merge_p2p(NumericEffectView<Decrease, FluentTag> element, MergeContext& context);
template std::pair<NumericEffectView<ScaleUp, FluentTag>, bool> merge_p2p(NumericEffectView<ScaleUp, FluentTag> element, MergeContext& context);
template std::pair<NumericEffectView<ScaleDown, FluentTag>, bool> merge_p2p(NumericEffectView<ScaleDown, FluentTag> element, MergeContext& context);

template std::pair<NumericEffectView<Increase, AuxiliaryTag>, bool> merge_p2p(NumericEffectView<Increase, AuxiliaryTag> element, MergeContext& context);

template NumericEffectOperatorView<FluentTag> merge_p2p(NumericEffectOperatorView<FluentTag> element, MergeContext& context);
template NumericEffectOperatorView<AuxiliaryTag> merge_p2p(NumericEffectOperatorView<AuxiliaryTag> element, MergeContext& context);

template std::pair<GroundNumericEffectView<Assign, FluentTag>, bool> merge_p2p(GroundNumericEffectView<Assign, FluentTag> element, MergeContext& context);
template std::pair<GroundNumericEffectView<Increase, FluentTag>, bool> merge_p2p(GroundNumericEffectView<Increase, FluentTag> element, MergeContext& context);
template std::pair<GroundNumericEffectView<Decrease, FluentTag>, bool> merge_p2p(GroundNumericEffectView<Decrease, FluentTag> element, MergeContext& context);
template std::pair<GroundNumericEffectView<ScaleUp, FluentTag>, bool> merge_p2p(GroundNumericEffectView<ScaleUp, FluentTag> element, MergeContext& context);
template std::pair<GroundNumericEffectView<ScaleDown, FluentTag>, bool> merge_p2p(GroundNumericEffectView<ScaleDown, FluentTag> element, MergeContext& context);

template std::pair<GroundNumericEffectView<Increase, AuxiliaryTag>, bool> merge_p2p(GroundNumericEffectView<Increase, AuxiliaryTag> element,
                                                                                    MergeContext& context);

template GroundNumericEffectOperatorView<FluentTag> merge_p2p(GroundNumericEffectOperatorView<FluentTag> element, MergeContext& context);
template GroundNumericEffectOperatorView<AuxiliaryTag> merge_p2p(GroundNumericEffectOperatorView<AuxiliaryTag> element, MergeContext& context);

}

#endif
