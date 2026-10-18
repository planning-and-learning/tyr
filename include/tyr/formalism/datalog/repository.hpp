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

#ifndef TYR_FORMALISM_DATALOG_REPOSITORY_HPP_
#define TYR_FORMALISM_DATALOG_REPOSITORY_HPP_

#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/datalog/datas.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/datalog/indices.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/predicate_view.hpp"

#include <cassert>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
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
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::formalism::datalog
{

using ygg::formalism::checkout;
using ygg::formalism::insert;

template<typename T>
void prepare_for_insert(Repository&, ygg::Data<T>& data)
{
    canonicalize(data);
}

template<RelationKind R>
std::optional<RuleView<GroundTag, R>> find_ground_rule(RuleBindingIndex<R> binding, const Repository& repository)
{
    auto rule = ygg::Data<Rule<GroundTag, R>> {};
    rule.binding = binding;
    return repository.find(rule);
}

template<RelationKind R>
std::optional<RuleView<GroundTag, R>> find_ground_rule(RuleBindingView<R> binding)
{
    return find_ground_rule(binding.get_index(), binding.get_context());
}

}

#ifndef TYR_HEADER_INSTANTIATION

extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Variable, true>;
extern template class ygg::formalism::BasicSymbolRepository<ygg::formalism::Object<tyr::formalism::ObjectTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Predicate<tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Predicate<tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Atom<tyr::LiftedTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Atom<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Atom<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Atom<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Literal<tyr::LiftedTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Literal<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Literal<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Literal<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Function<tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Function<tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::Function<tyr::formalism::AuxiliaryTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTerm<tyr::LiftedTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTerm<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTerm<tyr::LiftedTag, tyr::formalism::AuxiliaryTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTerm<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTerm<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTerm<tyr::GroundTag, tyr::formalism::AuxiliaryTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTermValue<tyr::GroundTag, tyr::formalism::StaticTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTermValue<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::FunctionTermValue<tyr::GroundTag, tyr::formalism::AuxiliaryTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::UnaryOperator<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::BinaryOperator<tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::MultiOperator<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::BinaryOperator<tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::UnaryOperator<tyr::GroundTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::BinaryOperator<tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::MultiOperator<tyr::GroundTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::BinaryOperator<tyr::GroundTag, tyr::formalism::BooleanOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::NumericEffect<tyr::LiftedTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::NumericEffect<tyr::GroundTag, tyr::formalism::FluentTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::ConjunctiveCondition<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::ConjunctiveEffect<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::ConditionalEffect<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Rule<tyr::LiftedTag, tyr::formalism::PredicateTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Rule<tyr::LiftedTag, tyr::formalism::FunctionTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::ConjunctiveCondition<tyr::GroundTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::ConjunctiveEffect<tyr::GroundTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::ConditionalEffect<tyr::GroundTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Rule<tyr::GroundTag, tyr::formalism::PredicateTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Rule<tyr::GroundTag, tyr::formalism::FunctionTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Program<tyr::LiftedTag>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::Program<tyr::GroundTag>, true>;

#endif

#endif
