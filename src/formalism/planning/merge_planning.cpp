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

template std::pair<PredicateView<StaticTag>, bool> merge_d2p(datalog::PredicateView<StaticTag> element, MergePlanningContext& context);
template std::pair<PredicateView<FluentTag>, bool> merge_d2p(datalog::PredicateView<FluentTag> element, MergePlanningContext& context);
template std::pair<PredicateView<DerivedTag>, bool> merge_d2p(datalog::PredicateView<FluentTag> element, MergePlanningContext& context);

template std::pair<AtomView<LiftedTag, StaticTag>, bool>
merge_d2p(datalog::AtomView<LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<AtomView<LiftedTag, FluentTag>, bool>
merge_d2p(datalog::AtomView<LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<AtomView<LiftedTag, DerivedTag>, bool>
merge_d2p(datalog::AtomView<LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

template std::pair<PredicateBindingView<StaticTag>, bool>
merge_d2p(datalog::PredicateBindingView<StaticTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<PredicateBindingView<FluentTag>, bool>
merge_d2p(datalog::PredicateBindingView<FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<PredicateBindingView<DerivedTag>, bool>
merge_d2p(datalog::PredicateBindingView<FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

template std::pair<AtomView<GroundTag, StaticTag>, bool>
merge_atom_d2p<StaticTag, StaticTag>(datalog::PredicateBindingView<StaticTag> element,
                                     const ygg::UnorderedMap<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
                                     MergePlanningContext& context);
template std::pair<AtomView<GroundTag, FluentTag>, bool>
merge_atom_d2p<FluentTag, FluentTag>(datalog::PredicateBindingView<FluentTag> element,
                                     const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
                                     MergePlanningContext& context);
template std::pair<AtomView<GroundTag, DerivedTag>, bool>
merge_atom_d2p<FluentTag, DerivedTag>(datalog::PredicateBindingView<FluentTag> element,
                                      const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
                                      MergePlanningContext& context);

template std::pair<AtomView<GroundTag, StaticTag>, bool>
merge_d2p(datalog::AtomView<GroundTag, StaticTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<AtomView<GroundTag, FluentTag>, bool>
merge_d2p(datalog::AtomView<GroundTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<AtomView<GroundTag, DerivedTag>, bool>
merge_d2p(datalog::AtomView<GroundTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

template std::pair<LiteralView<LiftedTag, StaticTag>, bool>
merge_d2p(datalog::LiteralView<LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<LiteralView<LiftedTag, FluentTag>, bool>
merge_d2p(datalog::LiteralView<LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<LiteralView<LiftedTag, DerivedTag>, bool>
merge_d2p(datalog::LiteralView<LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

template std::pair<LiteralView<GroundTag, StaticTag>, bool>
merge_d2p(datalog::LiteralView<GroundTag, StaticTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<StaticTag>, PredicateView<StaticTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<LiteralView<GroundTag, FluentTag>, bool>
merge_d2p(datalog::LiteralView<GroundTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<FluentTag>>& predicate_mapping,
          MergePlanningContext& context);
template std::pair<LiteralView<GroundTag, DerivedTag>, bool>
merge_d2p(datalog::LiteralView<GroundTag, FluentTag> element,
          const ygg::UnorderedMap<datalog::PredicateView<FluentTag>, PredicateView<DerivedTag>>& predicate_mapping,
          MergePlanningContext& context);

// Numeric

template std::pair<FunctionView<StaticTag>, bool> merge_d2p(datalog::FunctionView<StaticTag> element, MergePlanningContext& context);
template std::pair<FunctionView<FluentTag>, bool> merge_d2p(datalog::FunctionView<FluentTag> element, MergePlanningContext& context);

template std::pair<FunctionTermView<LiftedTag, StaticTag>, bool> merge_d2p(datalog::FunctionTermView<LiftedTag, StaticTag> element,
                                                                           MergePlanningContext& context);
template std::pair<FunctionTermView<LiftedTag, FluentTag>, bool> merge_d2p(datalog::FunctionTermView<LiftedTag, FluentTag> element,
                                                                           MergePlanningContext& context);

template std::pair<FunctionBindingView<StaticTag>, bool> merge_d2p(datalog::FunctionBindingView<StaticTag> element, MergePlanningContext& context);
template std::pair<FunctionBindingView<FluentTag>, bool> merge_d2p(datalog::FunctionBindingView<FluentTag> element, MergePlanningContext& context);

template std::pair<FunctionTermView<GroundTag, StaticTag>, bool> merge_d2p(datalog::FunctionTermView<GroundTag, StaticTag> element,
                                                                           MergePlanningContext& context);
template std::pair<FunctionTermView<GroundTag, FluentTag>, bool> merge_d2p(datalog::FunctionTermView<GroundTag, FluentTag> element,
                                                                           MergePlanningContext& context);

template std::pair<FunctionTermValueView<GroundTag, StaticTag>, bool> merge_d2p(datalog::FunctionTermValueView<GroundTag, StaticTag> element,
                                                                                MergePlanningContext& context);
template std::pair<FunctionTermValueView<GroundTag, FluentTag>, bool> merge_d2p(datalog::FunctionTermValueView<GroundTag, FluentTag> element,
                                                                                MergePlanningContext& context);

template std::pair<UnaryOperatorView<LiftedTag>, bool> merge_d2p(datalog::UnaryOperatorView<LiftedTag> element, MergePlanningContext& context);
template std::pair<UnaryOperatorView<GroundTag>, bool> merge_d2p(datalog::UnaryOperatorView<GroundTag> element, MergePlanningContext& context);

template std::pair<BinaryOperatorView<LiftedTag, BooleanOperatorKind>, bool> merge_d2p(datalog::BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                       MergePlanningContext& context);
template std::pair<BinaryOperatorView<LiftedTag, ArithmeticOperatorKind>, bool>
merge_d2p(datalog::BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element, MergePlanningContext& context);
template std::pair<BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> merge_d2p(datalog::BinaryOperatorView<GroundTag, BooleanOperatorKind> element,
                                                                                       MergePlanningContext& context);
template std::pair<BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool>
merge_d2p(datalog::BinaryOperatorView<GroundTag, ArithmeticOperatorKind> element, MergePlanningContext& context);

template std::pair<MultiOperatorView<LiftedTag>, bool> merge_d2p(datalog::MultiOperatorView<LiftedTag> element, MergePlanningContext& context);
template std::pair<MultiOperatorView<GroundTag>, bool> merge_d2p(datalog::MultiOperatorView<GroundTag> element, MergePlanningContext& context);

template ygg::Data<ArithmeticOperator<LiftedTag>> merge_d2p(datalog::ArithmeticOperatorView<LiftedTag> element, MergePlanningContext& context);
template ygg::Data<ArithmeticOperator<GroundTag>> merge_d2p(datalog::ArithmeticOperatorView<GroundTag> element, MergePlanningContext& context);

template ygg::Data<BooleanOperator<LiftedTag>> merge_d2p(datalog::BooleanOperatorView<LiftedTag> element, MergePlanningContext& context);
template ygg::Data<BooleanOperator<GroundTag>> merge_d2p(datalog::BooleanOperatorView<GroundTag> element, MergePlanningContext& context);

}

#endif
