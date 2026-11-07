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

#include "tyr/formalism/datalog/atom_data.hpp"
#include "tyr/formalism/datalog/binary_operator_data.hpp"
#include "tyr/formalism/datalog/conditional_effect_data.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_data.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_data.hpp"
#include "tyr/formalism/datalog/function_term_data.hpp"
#include "tyr/formalism/datalog/function_term_value_data.hpp"
#include "tyr/formalism/datalog/literal_data.hpp"
#include "tyr/formalism/datalog/multi_operator_data.hpp"
#include "tyr/formalism/datalog/numeric_effect_data.hpp"
#include "tyr/formalism/datalog/program_data.hpp"
#include "tyr/formalism/datalog/rule_data.hpp"
#include "tyr/formalism/datalog/unary_operator_data.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_data.hpp>
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/datalog/declarations.hpp"
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/datalog/atom_view.hpp"
#include "tyr/formalism/datalog/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conditional_effect_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/function_term_value_view.hpp"
#include "tyr/formalism/datalog/function_term_view.hpp"
#include "tyr/formalism/datalog/literal_view.hpp"
#include "tyr/formalism/datalog/multi_operator_view.hpp"
#include "tyr/formalism/datalog/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/datalog/unary_operator_view.hpp"
#include <yggdrasil/formalism/binding_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <cassert>
#include <optional>
#include <tuple>
#include <type_traits>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <utility>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/relation_repository.hpp>
#include <yggdrasil/formalism/repository.hpp>
#include <yggdrasil/formalism/repository_factory.hpp>
#include <yggdrasil/formalism/symbol_repository.hpp>

namespace tyr::formalism::datalog
{

using ygg::formalism::checkout;

template<RelationKind R>
std::optional<RuleView<GroundTag, R>> find_ground_rule(RuleBindingIndex<R> binding, const Repository& repository)
{
    auto rule = urd::data<Rule<GroundTag, R>> {};
    rule.binding = binding;
    return repository.find(rule);
}

template<RelationKind R>
std::optional<RuleView<GroundTag, R>> find_ground_rule(RuleBindingView<R> binding)
{ return find_ground_rule(binding.index(), binding.context()); }

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
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::UnaryOperator<tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::BinaryOperator<tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>,
                                                            true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::MultiOperator<tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::BinaryOperator<tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::UnaryOperator<tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::BinaryOperator<tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>,
                                                            true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::datalog::MultiOperator<tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>, true>;
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

#else

#include <yggdrasil/formalism/detail/basic_symbol_repository_impl.hpp>

#endif

#endif
