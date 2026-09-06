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

template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> ground(FunctionTermView<LiftedTag, StaticTag> element, GrounderContext& context);
template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> ground(FunctionTermView<LiftedTag, FluentTag> element, GrounderContext& context);
template std::pair<FunctionTermView<GroundTag, AuxiliaryTag>, bool> ground(FunctionTermView<LiftedTag, AuxiliaryTag> element,
                                                                                  GrounderContext& context);

template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool>
ground(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element, GrounderContext& context);
template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                           GrounderContext& context);

template std::pair<PredicateBindingView<StaticTag>, bool> ground(TermListView terms, PredicateView<StaticTag> predicate, GrounderContext& context);
template std::pair<PredicateBindingView<FluentTag>, bool> ground(TermListView terms, PredicateView<FluentTag> predicate, GrounderContext& context);
template std::pair<PredicateBindingView<DerivedTag>, bool> ground(TermListView terms, PredicateView<DerivedTag> predicate, GrounderContext& context);

template std::pair<AtomView<GroundTag, StaticTag>, bool> ground(AtomView<LiftedTag, StaticTag> element, GrounderContext& grounder_context);
template std::pair<AtomView<GroundTag, FluentTag>, bool> ground(AtomView<LiftedTag, FluentTag> element, GrounderContext& grounder_context);
template std::pair<AtomView<GroundTag, DerivedTag>, bool> ground(AtomView<LiftedTag, DerivedTag> element, GrounderContext& grounder_context);

template std::pair<LiteralView<GroundTag, StaticTag>, bool> ground(LiteralView<LiftedTag, StaticTag> element, GrounderContext& context);
template std::pair<LiteralView<GroundTag, FluentTag>, bool> ground(LiteralView<LiftedTag, FluentTag> element, GrounderContext& context);
template std::pair<LiteralView<GroundTag, DerivedTag>, bool> ground(LiteralView<LiftedTag, DerivedTag> element, GrounderContext& context);

template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> ground(NumericEffectView<LiftedTag, FluentTag> element,
                                                                                GrounderContext& context);
template std::pair<NumericEffectView<GroundTag, AuxiliaryTag>, bool> ground(NumericEffectView<LiftedTag, AuxiliaryTag> element,
                                                                                   GrounderContext& context);

template NumericEffectOperatorView<GroundTag, FluentTag> ground(NumericEffectOperatorView<LiftedTag, FluentTag> element,
                                                                       GrounderContext& context);
template NumericEffectOperatorView<GroundTag, AuxiliaryTag> ground(NumericEffectOperatorView<LiftedTag, AuxiliaryTag> element,
                                                                          GrounderContext& context);

template std::optional<FunctionTermView<GroundTag, StaticTag>> try_ground(FunctionTermView<LiftedTag, StaticTag> element,
                                                                                 GrounderContext& context);
template std::optional<FunctionTermView<GroundTag, FluentTag>> try_ground(FunctionTermView<LiftedTag, FluentTag> element,
                                                                                 GrounderContext& context);
template std::optional<FunctionTermView<GroundTag, AuxiliaryTag>> try_ground(FunctionTermView<LiftedTag, AuxiliaryTag> element,
                                                                                    GrounderContext& context);

template std::optional<AtomView<GroundTag, StaticTag>> try_ground(AtomView<LiftedTag, StaticTag> element, GrounderContext& context);
template std::optional<AtomView<GroundTag, FluentTag>> try_ground(AtomView<LiftedTag, FluentTag> element, GrounderContext& context);
template std::optional<AtomView<GroundTag, DerivedTag>> try_ground(AtomView<LiftedTag, DerivedTag> element, GrounderContext& context);
}

#endif
