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

#include "tyr/formalism/planning/grounder.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
template std::pair<FunctionBindingView<StaticTag>, bool> ground(TermListView terms, FunctionView<StaticTag> function, GrounderContext& context);
template std::pair<FunctionBindingView<FluentTag>, bool> ground(TermListView terms, FunctionView<FluentTag> function, GrounderContext& context);
template std::pair<FunctionBindingView<AuxiliaryTag>, bool> ground(TermListView terms, FunctionView<AuxiliaryTag> function, GrounderContext& context);

template std::pair<GroundFunctionTermView<StaticTag>, bool> ground(FunctionTermView<StaticTag> element, GrounderContext& context);
template std::pair<GroundFunctionTermView<FluentTag>, bool> ground(FunctionTermView<FluentTag> element, GrounderContext& context);
template std::pair<GroundFunctionTermView<AuxiliaryTag>, bool> ground(FunctionTermView<AuxiliaryTag> element, GrounderContext& context);

template std::pair<GroundBinaryOperatorView<ArithmeticOperatorKind>, bool> ground(LiftedBinaryOperatorView<ArithmeticOperatorKind> element,
                                                                                  GrounderContext& context);
template std::pair<GroundBinaryOperatorView<BooleanOperatorKind>, bool> ground(LiftedBinaryOperatorView<BooleanOperatorKind> element, GrounderContext& context);

template std::pair<PredicateBindingView<StaticTag>, bool> ground(TermListView terms, PredicateView<StaticTag> predicate, GrounderContext& context);
template std::pair<PredicateBindingView<FluentTag>, bool> ground(TermListView terms, PredicateView<FluentTag> predicate, GrounderContext& context);
template std::pair<PredicateBindingView<DerivedTag>, bool> ground(TermListView terms, PredicateView<DerivedTag> predicate, GrounderContext& context);

template std::pair<GroundAtomView<StaticTag>, bool> ground(AtomView<StaticTag> element, GrounderContext& grounder_context);
template std::pair<GroundAtomView<FluentTag>, bool> ground(AtomView<FluentTag> element, GrounderContext& grounder_context);
template std::pair<GroundAtomView<DerivedTag>, bool> ground(AtomView<DerivedTag> element, GrounderContext& grounder_context);

template std::pair<GroundLiteralView<StaticTag>, bool> ground(LiteralView<StaticTag> element, GrounderContext& context);
template std::pair<GroundLiteralView<FluentTag>, bool> ground(LiteralView<FluentTag> element, GrounderContext& context);
template std::pair<GroundLiteralView<DerivedTag>, bool> ground(LiteralView<DerivedTag> element, GrounderContext& context);

template std::pair<GroundNumericEffectView<FluentTag>, bool> ground(NumericEffectView<FluentTag> element, GrounderContext& context);
template std::pair<GroundNumericEffectView<AuxiliaryTag>, bool> ground(NumericEffectView<AuxiliaryTag> element, GrounderContext& context);

template GroundNumericEffectOperatorView<FluentTag> ground(NumericEffectOperatorView<FluentTag> element, GrounderContext& context);
template GroundNumericEffectOperatorView<AuxiliaryTag> ground(NumericEffectOperatorView<AuxiliaryTag> element, GrounderContext& context);

template std::optional<GroundFunctionTermView<StaticTag>> try_ground(FunctionTermView<StaticTag> element, GrounderContext& context);
template std::optional<GroundFunctionTermView<FluentTag>> try_ground(FunctionTermView<FluentTag> element, GrounderContext& context);
template std::optional<GroundFunctionTermView<AuxiliaryTag>> try_ground(FunctionTermView<AuxiliaryTag> element, GrounderContext& context);

template std::optional<GroundAtomView<StaticTag>> try_ground(AtomView<StaticTag> element, GrounderContext& context);
template std::optional<GroundAtomView<FluentTag>> try_ground(AtomView<FluentTag> element, GrounderContext& context);
template std::optional<GroundAtomView<DerivedTag>> try_ground(AtomView<DerivedTag> element, GrounderContext& context);
}

#endif
