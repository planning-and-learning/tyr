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

template std::pair<AtomView<::tyr::LiftedTag, StaticTag>, bool> merge_d2d(AtomView<::tyr::LiftedTag, StaticTag> element, MergeContext& context);
template std::pair<AtomView<::tyr::LiftedTag, FluentTag>, bool> merge_d2d(AtomView<::tyr::LiftedTag, FluentTag> element, MergeContext& context);

template std::pair<PredicateBindingView<StaticTag>, bool> merge_d2d(PredicateBindingView<StaticTag> element, MergeContext& context);
template std::pair<PredicateBindingView<FluentTag>, bool> merge_d2d(PredicateBindingView<FluentTag> element, MergeContext& context);

template std::pair<AtomView<::tyr::GroundTag, StaticTag>, bool> merge_d2d(AtomView<::tyr::GroundTag, StaticTag> element, MergeContext& context);
template std::pair<AtomView<::tyr::GroundTag, FluentTag>, bool> merge_d2d(AtomView<::tyr::GroundTag, FluentTag> element, MergeContext& context);

template std::pair<LiteralView<::tyr::LiftedTag, StaticTag>, bool> merge_d2d(LiteralView<::tyr::LiftedTag, StaticTag> element, MergeContext& context);
template std::pair<LiteralView<::tyr::LiftedTag, FluentTag>, bool> merge_d2d(LiteralView<::tyr::LiftedTag, FluentTag> element, MergeContext& context);

template std::pair<LiteralView<::tyr::GroundTag, StaticTag>, bool> merge_d2d(LiteralView<::tyr::GroundTag, StaticTag> element, MergeContext& context);
template std::pair<LiteralView<::tyr::GroundTag, FluentTag>, bool> merge_d2d(LiteralView<::tyr::GroundTag, FluentTag> element, MergeContext& context);

template std::pair<FunctionView<StaticTag>, bool> merge_d2d(FunctionView<StaticTag> element, MergeContext& context);
template std::pair<FunctionView<FluentTag>, bool> merge_d2d(FunctionView<FluentTag> element, MergeContext& context);

template std::pair<FunctionTermView<::tyr::LiftedTag, StaticTag>, bool> merge_d2d(FunctionTermView<::tyr::LiftedTag, StaticTag> element, MergeContext& context);
template std::pair<FunctionTermView<::tyr::LiftedTag, FluentTag>, bool> merge_d2d(FunctionTermView<::tyr::LiftedTag, FluentTag> element, MergeContext& context);

template std::pair<FunctionBindingView<StaticTag>, bool> merge_d2d(FunctionBindingView<StaticTag> element, MergeContext& context);
template std::pair<FunctionBindingView<FluentTag>, bool> merge_d2d(FunctionBindingView<FluentTag> element, MergeContext& context);

template std::pair<FunctionTermView<::tyr::GroundTag, StaticTag>, bool> merge_d2d(FunctionTermView<::tyr::GroundTag, StaticTag> element, MergeContext& context);
template std::pair<FunctionTermView<::tyr::GroundTag, FluentTag>, bool> merge_d2d(FunctionTermView<::tyr::GroundTag, FluentTag> element, MergeContext& context);

template std::pair<FunctionTermValueView<::tyr::GroundTag, StaticTag>, bool> merge_d2d(FunctionTermValueView<::tyr::GroundTag, StaticTag> element,
                                                                                       MergeContext& context);
template std::pair<FunctionTermValueView<::tyr::GroundTag, FluentTag>, bool> merge_d2d(FunctionTermValueView<::tyr::GroundTag, FluentTag> element,
                                                                                       MergeContext& context);

template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_d2d(UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_d2d(UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_d2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_d2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_d2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);
template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_d2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

template std::pair<MultiOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_d2d(MultiOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
template std::pair<MultiOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_d2d(MultiOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

template ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>
merge_d2d(ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
template ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>
merge_d2d(ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

template std::pair<NumericEffectView<::tyr::LiftedTag, FluentTag>, bool> merge_d2d(NumericEffectView<::tyr::LiftedTag, FluentTag> element,
                                                                                   MergeContext& context);
template NumericEffectOperatorView<::tyr::LiftedTag, FluentTag> merge_d2d(NumericEffectOperatorView<::tyr::LiftedTag, FluentTag> element,
                                                                          MergeContext& context);

template std::pair<NumericEffectView<::tyr::GroundTag, FluentTag>, bool> merge_d2d(NumericEffectView<::tyr::GroundTag, FluentTag> element,
                                                                                   MergeContext& context);
template NumericEffectOperatorView<::tyr::GroundTag, FluentTag> merge_d2d(NumericEffectOperatorView<::tyr::GroundTag, FluentTag> element,
                                                                          MergeContext& context);

template std::pair<RuleView<::tyr::LiftedTag, PredicateTag>, bool> merge_d2d(RuleView<::tyr::LiftedTag, PredicateTag> element, MergeContext& context);
template std::pair<RuleView<::tyr::LiftedTag, FunctionTag>, bool> merge_d2d(RuleView<::tyr::LiftedTag, FunctionTag> element, MergeContext& context);

template std::pair<RuleBindingView<PredicateTag>, bool> merge_d2d(RuleBindingView<PredicateTag> element, MergeContext& context);
template std::pair<RuleBindingView<FunctionTag>, bool> merge_d2d(RuleBindingView<FunctionTag> element, MergeContext& context);

template std::pair<RuleView<::tyr::GroundTag, PredicateTag>, bool> merge_d2d(RuleView<::tyr::GroundTag, PredicateTag> element, MergeContext& context);
template std::pair<RuleView<::tyr::GroundTag, FunctionTag>, bool> merge_d2d(RuleView<::tyr::GroundTag, FunctionTag> element, MergeContext& context);
}

#endif
