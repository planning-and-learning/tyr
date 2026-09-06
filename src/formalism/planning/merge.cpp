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

template std::pair<AtomView<::tyr::LiftedTag, StaticTag>, bool> merge_p2p(AtomView<::tyr::LiftedTag, StaticTag> element, MergeContext& context);
template std::pair<AtomView<::tyr::LiftedTag, FluentTag>, bool> merge_p2p(AtomView<::tyr::LiftedTag, FluentTag> element, MergeContext& context);
template std::pair<AtomView<::tyr::LiftedTag, DerivedTag>, bool> merge_p2p(AtomView<::tyr::LiftedTag, DerivedTag> element, MergeContext& context);

template std::pair<AtomView<::tyr::GroundTag, StaticTag>, bool> merge_p2p(AtomView<::tyr::GroundTag, StaticTag> element, MergeContext& context);
template std::pair<AtomView<::tyr::GroundTag, FluentTag>, bool> merge_p2p(AtomView<::tyr::GroundTag, FluentTag> element, MergeContext& context);
template std::pair<AtomView<::tyr::GroundTag, DerivedTag>, bool> merge_p2p(AtomView<::tyr::GroundTag, DerivedTag> element, MergeContext& context);

template std::pair<LiteralView<::tyr::LiftedTag, StaticTag>, bool> merge_p2p(LiteralView<::tyr::LiftedTag, StaticTag> element, MergeContext& context);
template std::pair<LiteralView<::tyr::LiftedTag, FluentTag>, bool> merge_p2p(LiteralView<::tyr::LiftedTag, FluentTag> element, MergeContext& context);
template std::pair<LiteralView<::tyr::LiftedTag, DerivedTag>, bool> merge_p2p(LiteralView<::tyr::LiftedTag, DerivedTag> element, MergeContext& context);

template std::pair<LiteralView<::tyr::GroundTag, StaticTag>, bool> merge_p2p(LiteralView<::tyr::GroundTag, StaticTag> element, MergeContext& context);
template std::pair<LiteralView<::tyr::GroundTag, FluentTag>, bool> merge_p2p(LiteralView<::tyr::GroundTag, FluentTag> element, MergeContext& context);
template std::pair<LiteralView<::tyr::GroundTag, DerivedTag>, bool> merge_p2p(LiteralView<::tyr::GroundTag, DerivedTag> element, MergeContext& context);

template std::pair<FunctionView<StaticTag>, bool> merge_p2p(FunctionView<StaticTag> element, MergeContext& context);
template std::pair<FunctionView<FluentTag>, bool> merge_p2p(FunctionView<FluentTag> element, MergeContext& context);
template std::pair<FunctionView<AuxiliaryTag>, bool> merge_p2p(FunctionView<AuxiliaryTag> element, MergeContext& context);

template std::pair<FunctionTermView<::tyr::LiftedTag, StaticTag>, bool> merge_p2p(FunctionTermView<::tyr::LiftedTag, StaticTag> element, MergeContext& context);
template std::pair<FunctionTermView<::tyr::LiftedTag, FluentTag>, bool> merge_p2p(FunctionTermView<::tyr::LiftedTag, FluentTag> element, MergeContext& context);
template std::pair<FunctionTermView<::tyr::LiftedTag, AuxiliaryTag>, bool> merge_p2p(FunctionTermView<::tyr::LiftedTag, AuxiliaryTag> element,
                                                                                     MergeContext& context);

template std::pair<FunctionTermView<::tyr::GroundTag, StaticTag>, bool> merge_p2p(FunctionTermView<::tyr::GroundTag, StaticTag> element, MergeContext& context);
template std::pair<FunctionTermView<::tyr::GroundTag, FluentTag>, bool> merge_p2p(FunctionTermView<::tyr::GroundTag, FluentTag> element, MergeContext& context);
template std::pair<FunctionTermView<::tyr::GroundTag, AuxiliaryTag>, bool> merge_p2p(FunctionTermView<::tyr::GroundTag, AuxiliaryTag> element,
                                                                                     MergeContext& context);

template std::pair<FunctionTermValueView<::tyr::GroundTag, StaticTag>, bool> merge_p2p(FunctionTermValueView<::tyr::GroundTag, StaticTag> element,
                                                                                       MergeContext& context);
template std::pair<FunctionTermValueView<::tyr::GroundTag, FluentTag>, bool> merge_p2p(FunctionTermValueView<::tyr::GroundTag, FluentTag> element,
                                                                                       MergeContext& context);
template std::pair<FunctionTermValueView<::tyr::GroundTag, AuxiliaryTag>, bool> merge_p2p(FunctionTermValueView<::tyr::GroundTag, AuxiliaryTag> element,
                                                                                          MergeContext& context);

template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_p2p(UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_p2p(UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_p2p(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_p2p(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_p2p(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);
template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_p2p(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

template std::pair<MultiOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_p2p(MultiOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
template std::pair<MultiOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_p2p(MultiOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

template ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>
merge_p2p(ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
template ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>
merge_p2p(ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

template BooleanOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>
merge_p2p(BooleanOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeContext& context);
template BooleanOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>
merge_p2p(BooleanOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeContext& context);

template std::pair<NumericEffectView<::tyr::LiftedTag, FluentTag>, bool> merge_p2p(NumericEffectView<::tyr::LiftedTag, FluentTag> element,
                                                                                   MergeContext& context);

template std::pair<NumericEffectView<::tyr::LiftedTag, AuxiliaryTag>, bool> merge_p2p(NumericEffectView<::tyr::LiftedTag, AuxiliaryTag> element,
                                                                                      MergeContext& context);

template NumericEffectOperatorView<::tyr::LiftedTag, FluentTag> merge_p2p(NumericEffectOperatorView<::tyr::LiftedTag, FluentTag> element,
                                                                          MergeContext& context);
template NumericEffectOperatorView<::tyr::LiftedTag, AuxiliaryTag> merge_p2p(NumericEffectOperatorView<::tyr::LiftedTag, AuxiliaryTag> element,
                                                                             MergeContext& context);

template std::pair<NumericEffectView<::tyr::GroundTag, FluentTag>, bool> merge_p2p(NumericEffectView<::tyr::GroundTag, FluentTag> element,
                                                                                   MergeContext& context);

template std::pair<NumericEffectView<::tyr::GroundTag, AuxiliaryTag>, bool> merge_p2p(NumericEffectView<::tyr::GroundTag, AuxiliaryTag> element,
                                                                                      MergeContext& context);

template NumericEffectOperatorView<::tyr::GroundTag, FluentTag> merge_p2p(NumericEffectOperatorView<::tyr::GroundTag, FluentTag> element,
                                                                          MergeContext& context);
template NumericEffectOperatorView<::tyr::GroundTag, AuxiliaryTag> merge_p2p(NumericEffectOperatorView<::tyr::GroundTag, AuxiliaryTag> element,
                                                                             MergeContext& context);

}

#endif
