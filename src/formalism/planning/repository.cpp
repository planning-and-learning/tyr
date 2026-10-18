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

#include "tyr/formalism/planning/repository.hpp"

namespace tyr::formalism
{
namespace planning
{
static_assert(RepositoryConcept<Repository>);
static_assert(Context<Repository>);
}
}

template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Variable, true>;
template class ygg::formalism::BasicSymbolRepository<ygg::formalism::Object<tyr::formalism::ObjectTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Predicate<tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Predicate<tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Predicate<tyr::formalism::DerivedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Atom<tyr::LiftedTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Atom<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Atom<tyr::LiftedTag, tyr::formalism::DerivedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Atom<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Atom<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Atom<tyr::GroundTag, tyr::formalism::DerivedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Literal<tyr::LiftedTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Literal<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Literal<tyr::LiftedTag, tyr::formalism::DerivedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Literal<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Literal<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Literal<tyr::GroundTag, tyr::formalism::DerivedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Function<tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Function<tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Function<tyr::formalism::AuxiliaryTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTerm<tyr::LiftedTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTerm<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTerm<tyr::LiftedTag, tyr::formalism::AuxiliaryTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTerm<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTerm<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTerm<tyr::GroundTag, tyr::formalism::AuxiliaryTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTermValue<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTermValue<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTermValue<tyr::GroundTag, tyr::formalism::AuxiliaryTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::UnaryOperator<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::BinaryOperator<tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::MultiOperator<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::BinaryOperator<tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::UnaryOperator<tyr::GroundTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::BinaryOperator<tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::MultiOperator<tyr::GroundTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::BinaryOperator<tyr::GroundTag, tyr::formalism::BooleanOperatorKind>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::NumericEffect<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::NumericEffect<tyr::LiftedTag, tyr::formalism::AuxiliaryTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::NumericEffect<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::NumericEffect<tyr::GroundTag, tyr::formalism::AuxiliaryTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::ConditionalEffect<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::ConditionalEffect<tyr::GroundTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::ConjunctiveEffect<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::ConjunctiveEffect<tyr::GroundTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Action<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Action<tyr::GroundTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Axiom<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Axiom<tyr::GroundTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Metric, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Domain, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Task<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Task<tyr::GroundTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FDRVariable<tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Invariant, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::ConjunctiveCondition<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::ConjunctiveCondition<tyr::GroundTag>, true>;
