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
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_data.hpp>
#include "tyr/formalism/function_data.hpp"
#include "tyr/formalism/object_data.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_data.hpp"
#include "tyr/formalism/variable_data.hpp"
#include "tyr/formalism/planning/action_data.hpp"
#include "tyr/formalism/planning/atom_data.hpp"
#include "tyr/formalism/planning/axiom_data.hpp"
#include "tyr/formalism/planning/binary_operator_data.hpp"
#include "tyr/formalism/planning/conditional_effect_data.hpp"
#include "tyr/formalism/planning/conjunctive_condition_data.hpp"
#include "tyr/formalism/planning/conjunctive_effect_data.hpp"
#include "tyr/formalism/planning/domain_data.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_variable_data.hpp"
#include "tyr/formalism/planning/function_term_data.hpp"
#include "tyr/formalism/planning/function_term_value_data.hpp"
#include "tyr/formalism/planning/invariant_data.hpp"
#include "tyr/formalism/planning/literal_data.hpp"
#include "tyr/formalism/planning/metric_data.hpp"
#include "tyr/formalism/planning/multi_operator_data.hpp"
#include "tyr/formalism/planning/numeric_effect_data.hpp"
#include "tyr/formalism/planning/task_data.hpp"
#include "tyr/formalism/planning/unary_operator_data.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/binding_index.hpp"
#include "tyr/formalism/planning/action_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/binary_operator_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/function_term_value_view.hpp"
#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/literal_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/multi_operator_view.hpp"
#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/planning/unary_operator_view.hpp"
#include <yggdrasil/formalism/binding_view.hpp>
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <cassert>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <yggdrasil/formalism/builder.hpp>
#include <yggdrasil/formalism/relation_repository.hpp>
#include <yggdrasil/formalism/repository.hpp>
#include <yggdrasil/formalism/repository_factory.hpp>
#include <yggdrasil/formalism/symbol_repository.hpp>

namespace tyr::formalism::planning
{

using ygg::formalism::checkout;

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
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::UnaryOperator<tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::BinaryOperator<tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>,
                                                            true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::MultiOperator<tyr::LiftedTag, tyr::formalism::ArithmeticOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::BinaryOperator<tyr::LiftedTag, tyr::formalism::BooleanOperatorKind>,
                                                            true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::UnaryOperator<tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::BinaryOperator<tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>,
                                                            true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::MultiOperator<tyr::GroundTag, tyr::formalism::ArithmeticOperatorKind>, true>;
extern template class ygg::formalism::BasicSymbolRepository<tyr::formalism::planning::BinaryOperator<tyr::GroundTag, tyr::formalism::BooleanOperatorKind>,
                                                            true>;
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
