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

template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression>>, bool> merge_d2d(UnaryOperatorView<ygg::Data<FunctionExpression>> element,
                                                                                     MergeContext& context);
template std::pair<UnaryOperatorView<ygg::Data<GroundFunctionExpression>>, bool> merge_d2d(UnaryOperatorView<ygg::Data<GroundFunctionExpression>> element,
                                                                                           MergeContext& context);

template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression>> element, MergeContext& context);
template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression>> element, MergeContext& context);
template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);
template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<GroundFunctionExpression>>, bool>
merge_d2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<GroundFunctionExpression>> element, MergeContext& context);

template std::pair<MultiOperatorView<ygg::Data<FunctionExpression>>, bool> merge_d2d(MultiOperatorView<ygg::Data<FunctionExpression>> element,
                                                                                     MergeContext& context);
template std::pair<MultiOperatorView<ygg::Data<GroundFunctionExpression>>, bool> merge_d2d(MultiOperatorView<ygg::Data<GroundFunctionExpression>> element,
                                                                                           MergeContext& context);

template ArithmeticOperatorView<ygg::Data<FunctionExpression>> merge_d2d(ArithmeticOperatorView<ygg::Data<FunctionExpression>> element, MergeContext& context);
template ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> merge_d2d(ArithmeticOperatorView<ygg::Data<GroundFunctionExpression>> element,
                                                                               MergeContext& context);

template std::pair<NumericEffectView<FluentTag>, bool> merge_d2d(NumericEffectView<FluentTag> element, MergeContext& context);
template NumericEffectOperatorView<FluentTag> merge_d2d(NumericEffectOperatorView<FluentTag> element, MergeContext& context);

template std::pair<GroundNumericEffectView<FluentTag>, bool> merge_d2d(GroundNumericEffectView<FluentTag> element, MergeContext& context);
template GroundNumericEffectOperatorView<FluentTag> merge_d2d(GroundNumericEffectOperatorView<FluentTag> element, MergeContext& context);

template std::pair<RuleView<PredicateTag>, bool> merge_d2d(RuleView<PredicateTag> element, MergeContext& context);
template std::pair<RuleView<FunctionTag>, bool> merge_d2d(RuleView<FunctionTag> element, MergeContext& context);

template std::pair<RuleBindingView<PredicateTag>, bool> merge_d2d(RuleBindingView<PredicateTag> element, MergeContext& context);
template std::pair<RuleBindingView<FunctionTag>, bool> merge_d2d(RuleBindingView<FunctionTag> element, MergeContext& context);

template std::pair<GroundRuleView<PredicateTag>, bool> merge_d2d(GroundRuleView<PredicateTag> element, MergeContext& context);
template std::pair<GroundRuleView<FunctionTag>, bool> merge_d2d(GroundRuleView<FunctionTag> element, MergeContext& context);
}

#endif
