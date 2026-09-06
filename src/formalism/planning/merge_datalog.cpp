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

#include "tyr/formalism/planning/merge_datalog.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::formalism::planning
{

template std::pair<datalog::PredicateView<StaticTag>, bool> merge_p2d(PredicateView<StaticTag> element, MergeDatalogContext& context);
template std::pair<datalog::PredicateView<FluentTag>, bool> merge_p2d(PredicateView<FluentTag> element, MergeDatalogContext& context);
template std::pair<datalog::PredicateView<FluentTag>, bool> merge_p2d(PredicateView<DerivedTag> element, MergeDatalogContext& context);

template std::pair<datalog::AtomView<LiftedTag, StaticTag>, bool>
merge_p2d(AtomView<LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<datalog::AtomView<LiftedTag, FluentTag>, bool>
merge_p2d(AtomView<LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<datalog::AtomView<LiftedTag, FluentTag>, bool>
merge_p2d(AtomView<LiftedTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<datalog::PredicateBindingView<StaticTag>, bool>
merge_p2d(PredicateBindingView<StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<datalog::PredicateBindingView<FluentTag>, bool>
merge_p2d(PredicateBindingView<FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<datalog::PredicateBindingView<FluentTag>, bool>
merge_p2d(PredicateBindingView<DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

template std::pair<datalog::AtomView<GroundTag, StaticTag>, bool>
merge_p2d(AtomView<GroundTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<datalog::AtomView<GroundTag, FluentTag>, bool>
merge_p2d(AtomView<GroundTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<datalog::AtomView<GroundTag, FluentTag>, bool>
merge_p2d(AtomView<GroundTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

template std::pair<datalog::LiteralView<LiftedTag, StaticTag>, bool>
merge_p2d(LiteralView<LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<datalog::LiteralView<LiftedTag, FluentTag>, bool>
merge_p2d(LiteralView<LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<datalog::LiteralView<LiftedTag, FluentTag>, bool>
merge_p2d(LiteralView<LiftedTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<datalog::LiteralView<GroundTag, StaticTag>, bool>
merge_p2d(LiteralView<GroundTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<datalog::LiteralView<GroundTag, FluentTag>, bool>
merge_p2d(LiteralView<GroundTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<datalog::LiteralView<GroundTag, FluentTag>, bool>
merge_p2d(LiteralView<GroundTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<datalog::LiteralView<GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::pair<datalog::ConjunctiveConditionView<GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<GroundTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

// Numeric

template std::pair<datalog::FunctionView<StaticTag>, bool> merge_p2d(FunctionView<StaticTag> element, MergeDatalogContext& context);
template std::pair<datalog::FunctionView<FluentTag>, bool> merge_p2d(FunctionView<FluentTag> element, MergeDatalogContext& context);

template std::pair<datalog::FunctionTermView<LiftedTag, StaticTag>, bool> merge_p2d(FunctionTermView<LiftedTag, StaticTag> element,
                                                                                    MergeDatalogContext& context);
template std::pair<datalog::FunctionTermView<LiftedTag, FluentTag>, bool> merge_p2d(FunctionTermView<LiftedTag, FluentTag> element,
                                                                                    MergeDatalogContext& context);

template std::pair<datalog::FunctionBindingView<StaticTag>, bool> merge_p2d(FunctionBindingView<StaticTag> element, MergeDatalogContext& context);
template std::pair<datalog::FunctionBindingView<FluentTag>, bool> merge_p2d(FunctionBindingView<FluentTag> element, MergeDatalogContext& context);

template std::pair<datalog::FunctionTermView<GroundTag, StaticTag>, bool> merge_p2d(FunctionTermView<GroundTag, StaticTag> element,
                                                                                    MergeDatalogContext& context);
template std::pair<datalog::FunctionTermView<GroundTag, FluentTag>, bool> merge_p2d(FunctionTermView<GroundTag, FluentTag> element,
                                                                                    MergeDatalogContext& context);

template std::pair<datalog::FunctionTermValueView<GroundTag, StaticTag>, bool> merge_p2d(FunctionTermValueView<GroundTag, StaticTag> element,
                                                                                         MergeDatalogContext& context);
template std::pair<datalog::FunctionTermValueView<GroundTag, FluentTag>, bool> merge_p2d(FunctionTermValueView<GroundTag, FluentTag> element,
                                                                                         MergeDatalogContext& context);

template std::pair<datalog::FunctionView<FluentTag>, bool> merge_p2d<AuxiliaryTag, FluentTag>(FunctionView<AuxiliaryTag> element, MergeDatalogContext& context);
template std::pair<datalog::FunctionTermView<LiftedTag, FluentTag>, bool> merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermView<LiftedTag, AuxiliaryTag> element,
                                                                                                             MergeDatalogContext& context);
template std::pair<datalog::FunctionBindingView<FluentTag>, bool> merge_p2d<AuxiliaryTag, FluentTag>(FunctionBindingView<AuxiliaryTag> element,
                                                                                                     MergeDatalogContext& context);
template std::pair<datalog::FunctionTermView<GroundTag, FluentTag>, bool> merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermView<GroundTag, AuxiliaryTag> element,
                                                                                                             MergeDatalogContext& context);
template std::pair<datalog::FunctionTermValueView<GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermValueView<GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);

template std::pair<datalog::NumericEffectView<LiftedTag, FluentTag>, bool> merge_p2d<FluentTag, FluentTag>(NumericEffectView<LiftedTag, FluentTag> element,
                                                                                                           MergeDatalogContext& context);
template std::pair<datalog::NumericEffectView<LiftedTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectView<LiftedTag, AuxiliaryTag> element, MergeDatalogContext& context);
template ygg::Data<datalog::NumericEffectOperator<LiftedTag, FluentTag>>
merge_p2d<FluentTag, FluentTag>(NumericEffectOperatorView<LiftedTag, FluentTag> element, MergeDatalogContext& context);
template ygg::Data<datalog::NumericEffectOperator<LiftedTag, FluentTag>>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectOperatorView<LiftedTag, AuxiliaryTag> element, MergeDatalogContext& context);

template std::pair<datalog::NumericEffectView<GroundTag, FluentTag>, bool> merge_p2d<FluentTag, FluentTag>(NumericEffectView<GroundTag, FluentTag> element,
                                                                                                           MergeDatalogContext& context);
template std::pair<datalog::NumericEffectView<GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectView<GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);
template ygg::Data<datalog::NumericEffectOperator<GroundTag, FluentTag>>
merge_p2d<FluentTag, FluentTag>(NumericEffectOperatorView<GroundTag, FluentTag> element, MergeDatalogContext& context);
template ygg::Data<datalog::NumericEffectOperator<GroundTag, FluentTag>>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectOperatorView<GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);

template std::pair<datalog::UnaryOperatorView<LiftedTag>, bool> merge_p2d(UnaryOperatorView<LiftedTag> element, MergeDatalogContext& context);
template std::pair<datalog::UnaryOperatorView<GroundTag>, bool> merge_p2d(UnaryOperatorView<GroundTag> element, MergeDatalogContext& context);

template std::pair<datalog::BinaryOperatorView<LiftedTag, BooleanOperatorKind>, bool> merge_p2d(BinaryOperatorView<LiftedTag, BooleanOperatorKind> element,
                                                                                                MergeDatalogContext& context);
template std::pair<datalog::BinaryOperatorView<LiftedTag, ArithmeticOperatorKind>, bool>
merge_p2d(BinaryOperatorView<LiftedTag, ArithmeticOperatorKind> element, MergeDatalogContext& context);
template std::pair<datalog::BinaryOperatorView<GroundTag, BooleanOperatorKind>, bool> merge_p2d(BinaryOperatorView<GroundTag, BooleanOperatorKind> element,
                                                                                                MergeDatalogContext& context);
template std::pair<datalog::BinaryOperatorView<GroundTag, ArithmeticOperatorKind>, bool>
merge_p2d(BinaryOperatorView<GroundTag, ArithmeticOperatorKind> element, MergeDatalogContext& context);

template std::pair<datalog::MultiOperatorView<LiftedTag>, bool> merge_p2d(MultiOperatorView<LiftedTag> element, MergeDatalogContext& context);
template std::pair<datalog::MultiOperatorView<GroundTag>, bool> merge_p2d(MultiOperatorView<GroundTag> element, MergeDatalogContext& context);

template ygg::Data<datalog::ArithmeticOperator<LiftedTag>> merge_p2d(ArithmeticOperatorView<LiftedTag> element, MergeDatalogContext& context);
template ygg::Data<datalog::ArithmeticOperator<GroundTag>> merge_p2d(ArithmeticOperatorView<GroundTag> element, MergeDatalogContext& context);

template ygg::Data<datalog::BooleanOperator<LiftedTag>> merge_p2d(BooleanOperatorView<LiftedTag> element, MergeDatalogContext& context);
template ygg::Data<datalog::BooleanOperator<GroundTag>> merge_p2d(BooleanOperatorView<GroundTag> element, MergeDatalogContext& context);

}

#endif
