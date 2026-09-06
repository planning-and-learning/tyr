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

template std::pair<AtomView<LiftedTag, StaticTag>, bool> merge_p2p(AtomView<LiftedTag, StaticTag> element, MergeContext& context);
template std::pair<AtomView<LiftedTag, FluentTag>, bool> merge_p2p(AtomView<LiftedTag, FluentTag> element, MergeContext& context);
template std::pair<AtomView<LiftedTag, DerivedTag>, bool> merge_p2p(AtomView<LiftedTag, DerivedTag> element, MergeContext& context);

template std::pair<AtomView<GroundTag, StaticTag>, bool> merge_p2p(AtomView<GroundTag, StaticTag> element, MergeContext& context);
template std::pair<AtomView<GroundTag, FluentTag>, bool> merge_p2p(AtomView<GroundTag, FluentTag> element, MergeContext& context);
template std::pair<AtomView<GroundTag, DerivedTag>, bool> merge_p2p(AtomView<GroundTag, DerivedTag> element, MergeContext& context);

template std::pair<LiteralView<LiftedTag, StaticTag>, bool> merge_p2p(LiteralView<LiftedTag, StaticTag> element, MergeContext& context);
template std::pair<LiteralView<LiftedTag, FluentTag>, bool> merge_p2p(LiteralView<LiftedTag, FluentTag> element, MergeContext& context);
template std::pair<LiteralView<LiftedTag, DerivedTag>, bool> merge_p2p(LiteralView<LiftedTag, DerivedTag> element, MergeContext& context);

template std::pair<LiteralView<GroundTag, StaticTag>, bool> merge_p2p(LiteralView<GroundTag, StaticTag> element, MergeContext& context);
template std::pair<LiteralView<GroundTag, FluentTag>, bool> merge_p2p(LiteralView<GroundTag, FluentTag> element, MergeContext& context);
template std::pair<LiteralView<GroundTag, DerivedTag>, bool> merge_p2p(LiteralView<GroundTag, DerivedTag> element, MergeContext& context);

template std::pair<FunctionView<StaticTag>, bool> merge_p2p(FunctionView<StaticTag> element, MergeContext& context);
template std::pair<FunctionView<FluentTag>, bool> merge_p2p(FunctionView<FluentTag> element, MergeContext& context);
template std::pair<FunctionView<AuxiliaryTag>, bool> merge_p2p(FunctionView<AuxiliaryTag> element, MergeContext& context);

template std::pair<FunctionTermView<LiftedTag, StaticTag>, bool> merge_p2p(FunctionTermView<LiftedTag, StaticTag> element, MergeContext& context);
template std::pair<FunctionTermView<LiftedTag, FluentTag>, bool> merge_p2p(FunctionTermView<LiftedTag, FluentTag> element, MergeContext& context);
template std::pair<FunctionTermView<LiftedTag, AuxiliaryTag>, bool> merge_p2p(FunctionTermView<LiftedTag, AuxiliaryTag> element,
                                                                                     MergeContext& context);

template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> merge_p2p(FunctionTermView<GroundTag, StaticTag> element, MergeContext& context);
template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> merge_p2p(FunctionTermView<GroundTag, FluentTag> element, MergeContext& context);
template std::pair<FunctionTermView<GroundTag, AuxiliaryTag>, bool> merge_p2p(FunctionTermView<GroundTag, AuxiliaryTag> element,
                                                                                     MergeContext& context);

template std::pair<FunctionTermValueView<GroundTag, StaticTag>, bool> merge_p2p(FunctionTermValueView<GroundTag, StaticTag> element,
                                                                                       MergeContext& context);
template std::pair<FunctionTermValueView<GroundTag, FluentTag>, bool> merge_p2p(FunctionTermValueView<GroundTag, FluentTag> element,
                                                                                       MergeContext& context);
template std::pair<FunctionTermValueView<GroundTag, AuxiliaryTag>, bool> merge_p2p(FunctionTermValueView<GroundTag, AuxiliaryTag> element,
                                                                                          MergeContext& context);

template std::pair<UnaryOperatorView<LiftedTag>, bool> merge_p2p(UnaryOperatorView<LiftedTag> element, MergeContext& context);
template std::pair<UnaryOperatorView<GroundTag>, bool> merge_p2p(UnaryOperatorView<GroundTag> element, MergeContext& context);

template std::pair<BinaryOperatorView<LiftedTag, BooleanOperatorKind>, bool> merge_p2p(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                              MergeContext& context);
template std::pair<BinaryOperatorView<LiftedTag, ArithmeticOperatorKind>, bool>
merge_p2p(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element, MergeContext& context);
template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> merge_p2p(BinaryOperatorView<GroundTag, BooleanOperatorKind> element,
                                                                                              MergeContext& context);
template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool>
merge_p2p(BinaryOperatorView<GroundTag, ArithmeticOperatorKind> element, MergeContext& context);

template std::pair<MultiOperatorView<LiftedTag>, bool> merge_p2p(MultiOperatorView<LiftedTag> element, MergeContext& context);
template std::pair<MultiOperatorView<GroundTag>, bool> merge_p2p(MultiOperatorView<GroundTag> element, MergeContext& context);

template ArithmeticOperatorView<LiftedTag> merge_p2p(ArithmeticOperatorView<LiftedTag> element, MergeContext& context);
template ArithmeticOperatorView<GroundTag> merge_p2p(ArithmeticOperatorView<GroundTag> element, MergeContext& context);

template BooleanOperatorView<LiftedTag> merge_p2p(BooleanOperatorView<LiftedTag> element, MergeContext& context);
template BooleanOperatorView<GroundTag> merge_p2p(BooleanOperatorView<GroundTag> element, MergeContext& context);

template std::pair<NumericEffectView<LiftedTag, FluentTag>, bool> merge_p2p(NumericEffectView<LiftedTag, FluentTag> element,
                                                                                   MergeContext& context);

template std::pair<NumericEffectView<LiftedTag, AuxiliaryTag>, bool> merge_p2p(NumericEffectView<LiftedTag, AuxiliaryTag> element,
                                                                                      MergeContext& context);

template NumericEffectOperatorView<LiftedTag, FluentTag> merge_p2p(NumericEffectOperatorView<LiftedTag, FluentTag> element,
                                                                          MergeContext& context);
template NumericEffectOperatorView<LiftedTag, AuxiliaryTag> merge_p2p(NumericEffectOperatorView<LiftedTag, AuxiliaryTag> element,
                                                                             MergeContext& context);

template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> merge_p2p(NumericEffectView<GroundTag, FluentTag> element,
                                                                                   MergeContext& context);

template std::pair<NumericEffectView<GroundTag, AuxiliaryTag>, bool> merge_p2p(NumericEffectView<GroundTag, AuxiliaryTag> element,
                                                                                      MergeContext& context);

template NumericEffectOperatorView<GroundTag, FluentTag> merge_p2p(NumericEffectOperatorView<GroundTag, FluentTag> element,
                                                                          MergeContext& context);
template NumericEffectOperatorView<GroundTag, AuxiliaryTag> merge_p2p(NumericEffectOperatorView<GroundTag, AuxiliaryTag> element,
                                                                             MergeContext& context);

}

#endif
