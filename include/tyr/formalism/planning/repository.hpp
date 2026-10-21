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

#ifndef TYR_FORMALISM_PLANNING_REPOSITORY_HPP_
#define TYR_FORMALISM_PLANNING_REPOSITORY_HPP_

#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/planning/canonicalization.hpp"
#include "tyr/formalism/planning/datas.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/indices.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <cassert>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/buffer/declarations.hpp>
#include <yggdrasil/buffer/indexed_hash_set.hpp>
#include <yggdrasil/buffer/segmented_buffer.hpp>
#include <yggdrasil/containers/tuple.hpp>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/interning.hpp>
#include <yggdrasil/formalism/relation_repository.hpp>
#include <yggdrasil/formalism/repository.hpp>
#include <yggdrasil/formalism/repository_factory.hpp>
#include <yggdrasil/formalism/symbol_repository.hpp>

namespace tyr::formalism::planning
{

using ygg::formalism::checkout;
using ygg::formalism::insert;

template<typename T>
void prepare_for_insert(Repository&, ygg::Data<T>& data)
{
    canonicalize(data);
}

}

#ifndef TYR_HEADER_INSTANTIATION

extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Variable, true>;
extern template class ygg::formalism::BasicSymbolRepository<ygg::formalism::Object<tyr::formalism::ObjectTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Predicate<tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Predicate<tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Predicate<tyr::formalism::DerivedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Atom<tyr::LiftedTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Atom<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Atom<tyr::LiftedTag, tyr::formalism::DerivedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Atom<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Atom<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Atom<tyr::GroundTag, tyr::formalism::DerivedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Literal<tyr::LiftedTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Literal<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Literal<tyr::LiftedTag, tyr::formalism::DerivedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Literal<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Literal<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Literal<tyr::GroundTag, tyr::formalism::DerivedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Function<tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Function<tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Function<tyr::formalism::AuxiliaryTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTerm<tyr::LiftedTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTerm<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTerm<tyr::LiftedTag, tyr::formalism::AuxiliaryTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTerm<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTerm<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTerm<tyr::GroundTag, tyr::formalism::AuxiliaryTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTermValue<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTermValue<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FunctionTermValue<tyr::GroundTag, tyr::formalism::AuxiliaryTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::UnaryOperator<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::BinaryOperator<tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::MultiOperator<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::BinaryOperator<tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::UnaryOperator<tyr::GroundTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::BinaryOperator<tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::MultiOperator<tyr::GroundTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::BinaryOperator<tyr::GroundTag, tyr::formalism::BooleanOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::NumericEffect<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::NumericEffect<tyr::LiftedTag, tyr::formalism::AuxiliaryTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::NumericEffect<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::NumericEffect<tyr::GroundTag, tyr::formalism::AuxiliaryTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::ConditionalEffect<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::ConditionalEffect<tyr::GroundTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::ConjunctiveEffect<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::ConjunctiveEffect<tyr::GroundTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Action<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Action<tyr::GroundTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Axiom<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Axiom<tyr::GroundTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Metric, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Domain, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Task<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Task<tyr::GroundTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::FDRVariable<tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::Invariant, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::ConjunctiveCondition<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::ConjunctiveCondition<tyr::GroundTag>, true>;

#else

#include <yggdrasil/formalism/detail/basic_symbol_repository_impl.hpp>

#endif

#endif
