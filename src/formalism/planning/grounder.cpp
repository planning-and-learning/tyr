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
#include "tyr/formalism/detail/repository_instantiations.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{
template std::pair<FunctionRowView<StaticTag>, bool> ground(TermListView terms, FunctionView<StaticTag> function, WriteContext<Builder, Repository>& context);
template std::pair<FunctionRowView<FluentTag>, bool> ground(TermListView terms, FunctionView<FluentTag> function, WriteContext<Builder, Repository>& context);
template std::pair<FunctionRowView<AuxiliaryTag>, bool> ground(TermListView terms, FunctionView<AuxiliaryTag> function, WriteContext<Builder, Repository>& context);

template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> ground(FunctionTermView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> ground(FunctionTermView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);
template std::pair<FunctionTermView<GroundTag, AuxiliaryTag>, bool> ground(FunctionTermView<LiftedTag, AuxiliaryTag> element, WriteContext<Builder, Repository>& context);

template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element,
                                                                                       WriteContext<Builder, Repository>& context);
template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> ground(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                    WriteContext<Builder, Repository>& context);

template std::pair<PredicateRowView<StaticTag>, bool> ground(TermListView terms, PredicateView<StaticTag> predicate, WriteContext<Builder, Repository>& context);
template std::pair<PredicateRowView<FluentTag>, bool> ground(TermListView terms, PredicateView<FluentTag> predicate, WriteContext<Builder, Repository>& context);
template std::pair<PredicateRowView<DerivedTag>, bool> ground(TermListView terms, PredicateView<DerivedTag> predicate, WriteContext<Builder, Repository>& context);

template std::pair<AtomView<GroundTag, StaticTag>, bool> ground(AtomView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& grounder_context);
template std::pair<AtomView<GroundTag, FluentTag>, bool> ground(AtomView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& grounder_context);
template std::pair<AtomView<GroundTag, DerivedTag>, bool> ground(AtomView<LiftedTag, DerivedTag> element, WriteContext<Builder, Repository>& grounder_context);

template std::pair<LiteralView<GroundTag, StaticTag>, bool> ground(LiteralView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
template std::pair<LiteralView<GroundTag, FluentTag>, bool> ground(LiteralView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);
template std::pair<LiteralView<GroundTag, DerivedTag>, bool> ground(LiteralView<LiftedTag, DerivedTag> element, WriteContext<Builder, Repository>& context);

template std::pair<NumericEffectView<GroundTag, FluentTag>, bool> ground(NumericEffectView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);
template std::pair<NumericEffectView<GroundTag, AuxiliaryTag>, bool> ground(NumericEffectView<LiftedTag, AuxiliaryTag> element, WriteContext<Builder, Repository>& context);

template std::optional<FunctionTermView<GroundTag, StaticTag>> try_ground(FunctionTermView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
template std::optional<FunctionTermView<GroundTag, FluentTag>> try_ground(FunctionTermView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);
template std::optional<FunctionTermView<GroundTag, AuxiliaryTag>> try_ground(FunctionTermView<LiftedTag, AuxiliaryTag> element, WriteContext<Builder, Repository>& context);

template std::optional<AtomView<GroundTag, StaticTag>> try_ground(AtomView<LiftedTag, StaticTag> element, WriteContext<Builder, Repository>& context);
template std::optional<AtomView<GroundTag, FluentTag>> try_ground(AtomView<LiftedTag, FluentTag> element, WriteContext<Builder, Repository>& context);
template std::optional<AtomView<GroundTag, DerivedTag>> try_ground(AtomView<LiftedTag, DerivedTag> element, WriteContext<Builder, Repository>& context);
}

#endif
