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

#include "tyr/formalism/datalog/copy.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::datalog
{
template std::pair<PredicateView<StaticTag>, bool> copy(PredicateView<StaticTag> element, CopyContext& context);
template std::pair<PredicateView<FluentTag>, bool> copy(PredicateView<FluentTag> element, CopyContext& context);

template std::pair<AtomView<LiftedTag, StaticTag>, bool> copy(AtomView<LiftedTag, StaticTag> element, CopyContext& context);
template std::pair<AtomView<LiftedTag, FluentTag>, bool> copy(AtomView<LiftedTag, FluentTag> element, CopyContext& context);

template std::pair<PredicateBindingView<StaticTag>, bool> copy(PredicateBindingView<StaticTag> element, CopyContext& context);
template std::pair<PredicateBindingView<FluentTag>, bool> copy(PredicateBindingView<FluentTag> element, CopyContext& context);

template std::pair<AtomView<GroundTag, StaticTag>, bool> copy(AtomView<GroundTag, StaticTag> element, CopyContext& context);
template std::pair<AtomView<GroundTag, FluentTag>, bool> copy(AtomView<GroundTag, FluentTag> element, CopyContext& context);

template std::pair<LiteralView<LiftedTag, StaticTag>, bool> copy(LiteralView<LiftedTag, StaticTag> element, CopyContext& context);
template std::pair<LiteralView<LiftedTag, FluentTag>, bool> copy(LiteralView<LiftedTag, FluentTag> element, CopyContext& context);

template std::pair<LiteralView<GroundTag, StaticTag>, bool> copy(LiteralView<GroundTag, StaticTag> element, CopyContext& context);
template std::pair<LiteralView<GroundTag, FluentTag>, bool> copy(LiteralView<GroundTag, FluentTag> element, CopyContext& context);

template std::pair<FunctionView<StaticTag>, bool> copy(FunctionView<StaticTag> element, CopyContext& context);
template std::pair<FunctionView<FluentTag>, bool> copy(FunctionView<FluentTag> element, CopyContext& context);

template std::pair<FunctionTermView<LiftedTag, StaticTag>, bool> copy(FunctionTermView<LiftedTag, StaticTag> element, CopyContext& context);
template std::pair<FunctionTermView<LiftedTag, FluentTag>, bool> copy(FunctionTermView<LiftedTag, FluentTag> element, CopyContext& context);

template std::pair<FunctionBindingView<StaticTag>, bool> copy(FunctionBindingView<StaticTag> element, CopyContext& context);
template std::pair<FunctionBindingView<FluentTag>, bool> copy(FunctionBindingView<FluentTag> element, CopyContext& context);

template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> copy(FunctionTermView<GroundTag, StaticTag> element, CopyContext& context);
template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> copy(FunctionTermView<GroundTag, FluentTag> element, CopyContext& context);

template std::pair<FunctionTermValueView<GroundTag, StaticTag>, bool> copy(FunctionTermValueView<GroundTag, StaticTag> element, CopyContext& context);
template std::pair<FunctionTermValueView<GroundTag, FluentTag>, bool> copy(FunctionTermValueView<GroundTag, FluentTag> element, CopyContext& context);

template std::pair<UnaryOperatorView<LiftedTag>, bool> copy(UnaryOperatorView<LiftedTag> element, CopyContext& context);
template std::pair<UnaryOperatorView<GroundTag>, bool> copy(UnaryOperatorView<GroundTag> element, CopyContext& context);

template std::pair<BinaryOperatorView<LiftedTag, BooleanOperatorKind>, bool> copy(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                  CopyContext& context);
template std::pair<BinaryOperatorView<LiftedTag, ArithmeticOperatorKind>, bool> copy(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                     CopyContext& context);
template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> copy(BinaryOperatorView<GroundTag, BooleanOperatorKind> element,
                                                                                  CopyContext& context);
template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> copy(BinaryOperatorView<GroundTag, ArithmeticOperatorKind> element,
                                                                                     CopyContext& context);

template std::pair<MultiOperatorView<LiftedTag>, bool> copy(MultiOperatorView<LiftedTag> element, CopyContext& context);
template std::pair<MultiOperatorView<GroundTag>, bool> copy(MultiOperatorView<GroundTag> element, CopyContext& context);

template ArithmeticOperatorView<LiftedTag> copy(ArithmeticOperatorView<LiftedTag> element, CopyContext& context);
template ArithmeticOperatorView<GroundTag> copy(ArithmeticOperatorView<GroundTag> element, CopyContext& context);

template std::pair<NumericEffectView<LiftedTag, FluentTag>, bool> copy(NumericEffectView<LiftedTag, FluentTag> element, CopyContext& context);
template NumericEffectOperatorView<LiftedTag, FluentTag> copy(NumericEffectOperatorView<LiftedTag, FluentTag> element, CopyContext& context);

template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> copy(NumericEffectView<GroundTag, FluentTag> element, CopyContext& context);
template NumericEffectOperatorView<GroundTag, FluentTag> copy(NumericEffectOperatorView<GroundTag, FluentTag> element, CopyContext& context);

template std::pair<RuleView<LiftedTag, PredicateTag>, bool> copy(RuleView<LiftedTag, PredicateTag> element, CopyContext& context);
template std::pair<RuleView<LiftedTag, FunctionTag>, bool> copy(RuleView<LiftedTag, FunctionTag> element, CopyContext& context);

template std::pair<RuleBindingView<PredicateTag>, bool> copy(RuleBindingView<PredicateTag> element, CopyContext& context);
template std::pair<RuleBindingView<FunctionTag>, bool> copy(RuleBindingView<FunctionTag> element, CopyContext& context);

template std::pair<RuleView<GroundTag, PredicateTag>, bool> copy(RuleView<GroundTag, PredicateTag> element, CopyContext& context);
template std::pair<RuleView<GroundTag, FunctionTag>, bool> copy(RuleView<GroundTag, FunctionTag> element, CopyContext& context);
}

#endif
