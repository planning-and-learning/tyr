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

#include "tyr/formalism/datalog/repository.hpp"

namespace tyr::formalism
{
namespace datalog
{
static_assert(RepositoryConcept<Repository>);
static_assert(Context<Repository>);
}
}

template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Atom<tyr::LiftedTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Atom<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Atom<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Atom<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Literal<tyr::LiftedTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Literal<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Literal<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Literal<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTerm<tyr::LiftedTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTerm<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTerm<tyr::LiftedTag, tyr::formalism::AuxiliaryTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTerm<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTerm<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTerm<tyr::GroundTag, tyr::formalism::AuxiliaryTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTermValue<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTermValue<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTermValue<tyr::GroundTag, tyr::formalism::AuxiliaryTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::UnaryOperator<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::BinaryOperator<tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::MultiOperator<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::BinaryOperator<tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::UnaryOperator<tyr::GroundTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::BinaryOperator<tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::MultiOperator<tyr::GroundTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::BinaryOperator<tyr::GroundTag, tyr::formalism::BooleanOperatorKind>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::NumericEffect<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::NumericEffect<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::ConjunctiveCondition<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::ConjunctiveEffect<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::ConditionalEffect<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Rule<tyr::LiftedTag, tyr::formalism::PredicateTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Rule<tyr::LiftedTag, tyr::formalism::FunctionTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::ConjunctiveCondition<tyr::GroundTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::ConjunctiveEffect<tyr::GroundTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::ConditionalEffect<tyr::GroundTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Rule<tyr::GroundTag, tyr::formalism::PredicateTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Rule<tyr::GroundTag, tyr::formalism::FunctionTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Program<tyr::LiftedTag>, true>;
template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Program<tyr::GroundTag>, true>;
