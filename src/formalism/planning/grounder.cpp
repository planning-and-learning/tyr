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

template std::pair<FunctionTermView<::tyr::GroundTag, StaticTag>, bool> ground(FunctionTermView<::tyr::LiftedTag, StaticTag> element, GrounderContext& context);
template std::pair<FunctionTermView<::tyr::GroundTag, FluentTag>, bool> ground(FunctionTermView<::tyr::LiftedTag, FluentTag> element, GrounderContext& context);
template std::pair<FunctionTermView<::tyr::GroundTag, AuxiliaryTag>, bool> ground(FunctionTermView<::tyr::LiftedTag, AuxiliaryTag> element,
                                                                                  GrounderContext& context);

template std::pair<GroundBinaryOperatorView<ArithmeticOperatorKind>, bool> ground(LiftedBinaryOperatorView<ArithmeticOperatorKind> element,
                                                                                  GrounderContext& context);
template std::pair<GroundBinaryOperatorView<BooleanOperatorKind>, bool> ground(LiftedBinaryOperatorView<BooleanOperatorKind> element, GrounderContext& context);

template std::pair<PredicateBindingView<StaticTag>, bool> ground(TermListView terms, PredicateView<StaticTag> predicate, GrounderContext& context);
template std::pair<PredicateBindingView<FluentTag>, bool> ground(TermListView terms, PredicateView<FluentTag> predicate, GrounderContext& context);
template std::pair<PredicateBindingView<DerivedTag>, bool> ground(TermListView terms, PredicateView<DerivedTag> predicate, GrounderContext& context);

template std::pair<AtomView<::tyr::GroundTag, StaticTag>, bool> ground(AtomView<::tyr::LiftedTag, StaticTag> element, GrounderContext& grounder_context);
template std::pair<AtomView<::tyr::GroundTag, FluentTag>, bool> ground(AtomView<::tyr::LiftedTag, FluentTag> element, GrounderContext& grounder_context);
template std::pair<AtomView<::tyr::GroundTag, DerivedTag>, bool> ground(AtomView<::tyr::LiftedTag, DerivedTag> element, GrounderContext& grounder_context);

template std::pair<LiteralView<::tyr::GroundTag, StaticTag>, bool> ground(LiteralView<::tyr::LiftedTag, StaticTag> element, GrounderContext& context);
template std::pair<LiteralView<::tyr::GroundTag, FluentTag>, bool> ground(LiteralView<::tyr::LiftedTag, FluentTag> element, GrounderContext& context);
template std::pair<LiteralView<::tyr::GroundTag, DerivedTag>, bool> ground(LiteralView<::tyr::LiftedTag, DerivedTag> element, GrounderContext& context);

template std::pair<NumericEffectView<::tyr::GroundTag, FluentTag>, bool> ground(NumericEffectView<::tyr::LiftedTag, FluentTag> element,
                                                                                GrounderContext& context);
template std::pair<NumericEffectView<::tyr::GroundTag, AuxiliaryTag>, bool> ground(NumericEffectView<::tyr::LiftedTag, AuxiliaryTag> element,
                                                                                   GrounderContext& context);

template NumericEffectOperatorView<::tyr::GroundTag, FluentTag> ground(NumericEffectOperatorView<::tyr::LiftedTag, FluentTag> element,
                                                                       GrounderContext& context);
template NumericEffectOperatorView<::tyr::GroundTag, AuxiliaryTag> ground(NumericEffectOperatorView<::tyr::LiftedTag, AuxiliaryTag> element,
                                                                          GrounderContext& context);

template std::optional<FunctionTermView<::tyr::GroundTag, StaticTag>> try_ground(FunctionTermView<::tyr::LiftedTag, StaticTag> element,
                                                                                 GrounderContext& context);
template std::optional<FunctionTermView<::tyr::GroundTag, FluentTag>> try_ground(FunctionTermView<::tyr::LiftedTag, FluentTag> element,
                                                                                 GrounderContext& context);
template std::optional<FunctionTermView<::tyr::GroundTag, AuxiliaryTag>> try_ground(FunctionTermView<::tyr::LiftedTag, AuxiliaryTag> element,
                                                                                    GrounderContext& context);

template std::optional<AtomView<::tyr::GroundTag, StaticTag>> try_ground(AtomView<::tyr::LiftedTag, StaticTag> element, GrounderContext& context);
template std::optional<AtomView<::tyr::GroundTag, FluentTag>> try_ground(AtomView<::tyr::LiftedTag, FluentTag> element, GrounderContext& context);
template std::optional<AtomView<::tyr::GroundTag, DerivedTag>> try_ground(AtomView<::tyr::LiftedTag, DerivedTag> element, GrounderContext& context);
}

#endif
