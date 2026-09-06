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

template std::pair<AtomView<LiftedTag, StaticTag>, bool> merge_d2d(AtomView<LiftedTag, StaticTag> element, MergeContext& context);
template std::pair<AtomView<LiftedTag, FluentTag>, bool> merge_d2d(AtomView<LiftedTag, FluentTag> element, MergeContext& context);

template std::pair<PredicateBindingView<StaticTag>, bool> merge_d2d(PredicateBindingView<StaticTag> element, MergeContext& context);
template std::pair<PredicateBindingView<FluentTag>, bool> merge_d2d(PredicateBindingView<FluentTag> element, MergeContext& context);

template std::pair<AtomView<GroundTag, StaticTag>, bool> merge_d2d(AtomView<GroundTag, StaticTag> element, MergeContext& context);
template std::pair<AtomView<GroundTag, FluentTag>, bool> merge_d2d(AtomView<GroundTag, FluentTag> element, MergeContext& context);

template std::pair<LiteralView<LiftedTag, StaticTag>, bool> merge_d2d(LiteralView<LiftedTag, StaticTag> element, MergeContext& context);
template std::pair<LiteralView<LiftedTag, FluentTag>, bool> merge_d2d(LiteralView<LiftedTag, FluentTag> element, MergeContext& context);

template std::pair<LiteralView<GroundTag, StaticTag>, bool> merge_d2d(LiteralView<GroundTag, StaticTag> element, MergeContext& context);
template std::pair<LiteralView<GroundTag, FluentTag>, bool> merge_d2d(LiteralView<GroundTag, FluentTag> element, MergeContext& context);

template std::pair<FunctionView<StaticTag>, bool> merge_d2d(FunctionView<StaticTag> element, MergeContext& context);
template std::pair<FunctionView<FluentTag>, bool> merge_d2d(FunctionView<FluentTag> element, MergeContext& context);

template std::pair<FunctionTermView<LiftedTag, StaticTag>, bool> merge_d2d(FunctionTermView<LiftedTag, StaticTag> element, MergeContext& context);
template std::pair<FunctionTermView<LiftedTag, FluentTag>, bool> merge_d2d(FunctionTermView<LiftedTag, FluentTag> element, MergeContext& context);

template std::pair<FunctionBindingView<StaticTag>, bool> merge_d2d(FunctionBindingView<StaticTag> element, MergeContext& context);
template std::pair<FunctionBindingView<FluentTag>, bool> merge_d2d(FunctionBindingView<FluentTag> element, MergeContext& context);

template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> merge_d2d(FunctionTermView<GroundTag, StaticTag> element, MergeContext& context);
template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> merge_d2d(FunctionTermView<GroundTag, FluentTag> element, MergeContext& context);

template std::pair<FunctionTermValueView<GroundTag, StaticTag>, bool> merge_d2d(FunctionTermValueView<GroundTag, StaticTag> element, MergeContext& context);
template std::pair<FunctionTermValueView<GroundTag, FluentTag>, bool> merge_d2d(FunctionTermValueView<GroundTag, FluentTag> element, MergeContext& context);

template std::pair<UnaryOperatorView<LiftedTag>, bool> merge_d2d(UnaryOperatorView<LiftedTag> element, MergeContext& context);
template std::pair<UnaryOperatorView<GroundTag>, bool> merge_d2d(UnaryOperatorView<GroundTag> element, MergeContext& context);

template std::pair<BinaryOperatorView<LiftedTag, BooleanOperatorKind>, bool> merge_d2d(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                       MergeContext& context);
template std::pair<BinaryOperatorView<LiftedTag, ArithmeticOperatorKind>, bool> merge_d2d(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                          MergeContext& context);
template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> merge_d2d(BinaryOperatorView<GroundTag, BooleanOperatorKind> element,
                                                                                       MergeContext& context);
template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> merge_d2d(BinaryOperatorView<GroundTag, ArithmeticOperatorKind> element,
                                                                                          MergeContext& context);

template std::pair<MultiOperatorView<LiftedTag>, bool> merge_d2d(MultiOperatorView<LiftedTag> element, MergeContext& context);
template std::pair<MultiOperatorView<GroundTag>, bool> merge_d2d(MultiOperatorView<GroundTag> element, MergeContext& context);

template ArithmeticOperatorView<LiftedTag> merge_d2d(ArithmeticOperatorView<LiftedTag> element, MergeContext& context);
template ArithmeticOperatorView<GroundTag> merge_d2d(ArithmeticOperatorView<GroundTag> element, MergeContext& context);

template std::pair<NumericEffectView<LiftedTag, FluentTag>, bool> merge_d2d(NumericEffectView<LiftedTag, FluentTag> element, MergeContext& context);
template NumericEffectOperatorView<LiftedTag, FluentTag> merge_d2d(NumericEffectOperatorView<LiftedTag, FluentTag> element, MergeContext& context);

template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> merge_d2d(NumericEffectView<GroundTag, FluentTag> element, MergeContext& context);
template NumericEffectOperatorView<GroundTag, FluentTag> merge_d2d(NumericEffectOperatorView<GroundTag, FluentTag> element, MergeContext& context);

template std::pair<RuleView<LiftedTag, PredicateTag>, bool> merge_d2d(RuleView<LiftedTag, PredicateTag> element, MergeContext& context);
template std::pair<RuleView<LiftedTag, FunctionTag>, bool> merge_d2d(RuleView<LiftedTag, FunctionTag> element, MergeContext& context);

template std::pair<RuleBindingView<PredicateTag>, bool> merge_d2d(RuleBindingView<PredicateTag> element, MergeContext& context);
template std::pair<RuleBindingView<FunctionTag>, bool> merge_d2d(RuleBindingView<FunctionTag> element, MergeContext& context);

template std::pair<RuleView<GroundTag, PredicateTag>, bool> merge_d2d(RuleView<GroundTag, PredicateTag> element, MergeContext& context);
template std::pair<RuleView<GroundTag, FunctionTag>, bool> merge_d2d(RuleView<GroundTag, FunctionTag> element, MergeContext& context);
}

#endif
