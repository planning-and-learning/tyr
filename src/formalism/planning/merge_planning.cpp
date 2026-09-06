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

#include "tyr/formalism/planning/merge_planning.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{

template std::pair<PredicateView<StaticTag>, bool> merge_d2p(::tyr::formalism::datalog::PredicateView<StaticTag> element, MergePlanningContext& context);
template std::pair<PredicateView<FluentTag>, bool> merge_d2p(::tyr::formalism::datalog::PredicateView<FluentTag> element, MergePlanningContext& context);
template std::pair<PredicateView<DerivedTag>, bool> merge_d2p(::tyr::formalism::datalog::PredicateView<FluentTag> element, MergePlanningContext& context);

template std::pair<AtomView<::tyr::LiftedTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<AtomView<::tyr::LiftedTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<AtomView<::tyr::LiftedTag, DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

template std::pair<PredicateBindingView<StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::PredicateBindingView<StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<PredicateBindingView<FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::PredicateBindingView<FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<PredicateBindingView<DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::PredicateBindingView<FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

template std::pair<AtomView<::tyr::GroundTag, StaticTag>, bool>
merge_atom_d2p<StaticTag, StaticTag>(::tyr::formalism::datalog::PredicateBindingView<StaticTag> element,
                                     const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
                                     MergePlanningContext& context);
template std::pair<AtomView<::tyr::GroundTag, FluentTag>, bool>
merge_atom_d2p<FluentTag, FluentTag>(::tyr::formalism::datalog::PredicateBindingView<FluentTag> element,
                                     const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
                                     MergePlanningContext& context);
template std::pair<AtomView<::tyr::GroundTag, DerivedTag>, bool> merge_atom_d2p<FluentTag, DerivedTag>(
    ::tyr::formalism::datalog::PredicateBindingView<FluentTag> element,
    const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
    MergePlanningContext& context);

template std::pair<AtomView<::tyr::GroundTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::GroundTag, StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<AtomView<::tyr::GroundTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<AtomView<::tyr::GroundTag, DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

template std::pair<LiteralView<::tyr::LiftedTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<LiteralView<::tyr::LiftedTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<LiteralView<::tyr::LiftedTag, DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

template std::pair<LiteralView<::tyr::GroundTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, StaticTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<LiteralView<::tyr::GroundTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<LiteralView<::tyr::GroundTag, DerivedTag>, bool>
merge_d2p(::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag> element,
          const ygg::UnorderedMap<::tyr::formalism::datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

// Numeric

template std::pair<FunctionView<StaticTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionView<StaticTag> element, MergePlanningContext& context);
template std::pair<FunctionView<FluentTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionView<FluentTag> element, MergePlanningContext& context);

template std::pair<FunctionTermView<::tyr::LiftedTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, StaticTag> element, MergePlanningContext& context);
template std::pair<FunctionTermView<::tyr::LiftedTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, FluentTag> element, MergePlanningContext& context);

template std::pair<FunctionBindingView<StaticTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionBindingView<StaticTag> element,
                                                                   MergePlanningContext& context);
template std::pair<FunctionBindingView<FluentTag>, bool> merge_d2p(::tyr::formalism::datalog::FunctionBindingView<FluentTag> element,
                                                                   MergePlanningContext& context);

template std::pair<FunctionTermView<::tyr::GroundTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, StaticTag> element, MergePlanningContext& context);
template std::pair<FunctionTermView<::tyr::GroundTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, FluentTag> element, MergePlanningContext& context);

template std::pair<FunctionTermValueView<::tyr::GroundTag, StaticTag>, bool>
merge_d2p(::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, StaticTag> element, MergePlanningContext& context);
template std::pair<FunctionTermValueView<::tyr::GroundTag, FluentTag>, bool>
merge_d2p(::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, FluentTag> element, MergePlanningContext& context);

template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_d2p(::tyr::formalism::datalog::UnaryOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>> element,
          MergePlanningContext& context);
template std::pair<UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_d2p(::tyr::formalism::datalog::UnaryOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>> element,
          MergePlanningContext& context);

template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool> merge_d2p(
    ::tyr::formalism::datalog::BinaryOperatorView<BooleanOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>> element,
    MergePlanningContext& context);
template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool> merge_d2p(
    ::tyr::formalism::datalog::BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>> element,
    MergePlanningContext& context);
template std::pair<BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool> merge_d2p(
    ::tyr::formalism::datalog::BinaryOperatorView<BooleanOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>> element,
    MergePlanningContext& context);
template std::pair<BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool> merge_d2p(
    ::tyr::formalism::datalog::BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>> element,
    MergePlanningContext& context);

template std::pair<MultiOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_d2p(::tyr::formalism::datalog::MultiOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>> element,
          MergePlanningContext& context);
template std::pair<MultiOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>>, bool>
merge_d2p(::tyr::formalism::datalog::MultiOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>> element,
          MergePlanningContext& context);

template ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>
merge_d2p(::tyr::formalism::datalog::ArithmeticOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>> element,
          MergePlanningContext& context);
template ygg::Data<ArithmeticOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>
merge_d2p(::tyr::formalism::datalog::ArithmeticOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>> element,
          MergePlanningContext& context);

template ygg::Data<BooleanOperator<ygg::Data<FunctionExpression<::tyr::LiftedTag>>>>
merge_d2p(::tyr::formalism::datalog::BooleanOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>> element,
          MergePlanningContext& context);
template ygg::Data<BooleanOperator<ygg::Data<FunctionExpression<::tyr::GroundTag>>>>
merge_d2p(::tyr::formalism::datalog::BooleanOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>> element,
          MergePlanningContext& context);

}

#endif
