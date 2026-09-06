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

template std::pair<::tyr::formalism::datalog::PredicateView<StaticTag>, bool> merge_p2d(PredicateView<StaticTag> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::PredicateView<FluentTag>, bool> merge_p2d(PredicateView<FluentTag> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::PredicateView<FluentTag>, bool> merge_p2d(PredicateView<DerivedTag> element, MergeDatalogContext& context);

template std::pair<::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, StaticTag>, bool>
merge_p2d(AtomView<::tyr::LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d(AtomView<::tyr::LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::AtomView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d(AtomView<::tyr::LiftedTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::PredicateBindingView<StaticTag>, bool>
merge_p2d(PredicateBindingView<StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::PredicateBindingView<FluentTag>, bool>
merge_p2d(PredicateBindingView<FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::PredicateBindingView<FluentTag>, bool>
merge_p2d(PredicateBindingView<DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

template std::pair<::tyr::formalism::datalog::AtomView<::tyr::GroundTag, StaticTag>, bool>
merge_p2d(AtomView<::tyr::GroundTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d(AtomView<::tyr::GroundTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::AtomView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d(AtomView<::tyr::GroundTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

template std::pair<::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, StaticTag>, bool>
merge_p2d(LiteralView<::tyr::LiftedTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d(LiteralView<::tyr::LiftedTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::LiteralView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d(LiteralView<::tyr::LiftedTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, StaticTag>, bool>
merge_p2d(LiteralView<::tyr::GroundTag, StaticTag> element,
          const ygg::UnorderedMap<PredicateView<StaticTag>, ::tyr::formalism::datalog::PredicateView<StaticTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d(LiteralView<::tyr::GroundTag, FluentTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d(LiteralView<::tyr::GroundTag, DerivedTag> element,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::optional<::tyr::formalism::datalog::LiteralView<::tyr::GroundTag, FluentTag>>
merge_p2d(FDRFactView<FluentTag> element,
          bool polarity,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& predicate_mapping,
          MergeDatalogContext& context);

std::pair<::tyr::formalism::datalog::ConjunctiveConditionView<::tyr::GroundTag>, bool>
merge_p2d(ConjunctiveConditionView<::tyr::GroundTag> element,
          const ygg::UnorderedMap<PredicateView<FluentTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& fluent_predicate_mapping,
          const ygg::UnorderedMap<PredicateView<DerivedTag>, ::tyr::formalism::datalog::PredicateView<FluentTag>>& derived_predicate_mapping,
          MergeDatalogContext& context);

// Numeric

template std::pair<::tyr::formalism::datalog::FunctionView<StaticTag>, bool> merge_p2d(FunctionView<StaticTag> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::FunctionView<FluentTag>, bool> merge_p2d(FunctionView<FluentTag> element, MergeDatalogContext& context);

template std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, StaticTag>, bool>
merge_p2d(FunctionTermView<::tyr::LiftedTag, StaticTag> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d(FunctionTermView<::tyr::LiftedTag, FluentTag> element, MergeDatalogContext& context);

template std::pair<::tyr::formalism::datalog::FunctionBindingView<StaticTag>, bool> merge_p2d(FunctionBindingView<StaticTag> element,
                                                                                              MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::FunctionBindingView<FluentTag>, bool> merge_p2d(FunctionBindingView<FluentTag> element,
                                                                                              MergeDatalogContext& context);

template std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, StaticTag>, bool>
merge_p2d(FunctionTermView<::tyr::GroundTag, StaticTag> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d(FunctionTermView<::tyr::GroundTag, FluentTag> element, MergeDatalogContext& context);

template std::pair<::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, StaticTag>, bool>
merge_p2d(FunctionTermValueView<::tyr::GroundTag, StaticTag> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d(FunctionTermValueView<::tyr::GroundTag, FluentTag> element, MergeDatalogContext& context);

template std::pair<::tyr::formalism::datalog::FunctionView<FluentTag>, bool> merge_p2d<AuxiliaryTag, FluentTag>(FunctionView<AuxiliaryTag> element,
                                                                                                                MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermView<::tyr::LiftedTag, AuxiliaryTag> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::FunctionBindingView<FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionBindingView<AuxiliaryTag> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::FunctionTermView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermView<::tyr::GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::FunctionTermValueView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(FunctionTermValueView<::tyr::GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);

template std::pair<::tyr::formalism::datalog::NumericEffectView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d<FluentTag, FluentTag>(NumericEffectView<::tyr::LiftedTag, FluentTag> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::NumericEffectView<::tyr::LiftedTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectView<::tyr::LiftedTag, AuxiliaryTag> element, MergeDatalogContext& context);
template ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, FluentTag>>
merge_p2d<FluentTag, FluentTag>(NumericEffectOperatorView<::tyr::LiftedTag, FluentTag> element, MergeDatalogContext& context);
template ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::LiftedTag, FluentTag>>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectOperatorView<::tyr::LiftedTag, AuxiliaryTag> element, MergeDatalogContext& context);

template std::pair<::tyr::formalism::datalog::NumericEffectView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d<FluentTag, FluentTag>(NumericEffectView<::tyr::GroundTag, FluentTag> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::NumericEffectView<::tyr::GroundTag, FluentTag>, bool>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectView<::tyr::GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);
template ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, FluentTag>>
merge_p2d<FluentTag, FluentTag>(NumericEffectOperatorView<::tyr::GroundTag, FluentTag> element, MergeDatalogContext& context);
template ygg::Data<::tyr::formalism::datalog::NumericEffectOperator<::tyr::GroundTag, FluentTag>>
merge_p2d<AuxiliaryTag, FluentTag>(NumericEffectOperatorView<::tyr::GroundTag, AuxiliaryTag> element, MergeDatalogContext& context);

template std::pair<::tyr::formalism::datalog::UnaryOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_p2d(UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::UnaryOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>>, bool>
merge_p2d(UnaryOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeDatalogContext& context);

template std::
    pair<::tyr::formalism::datalog::BinaryOperatorView<BooleanOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>>, bool>
    merge_p2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeDatalogContext& context);
template std::pair<
    ::tyr::formalism::datalog::BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>>,
    bool>
merge_p2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeDatalogContext& context);
template std::
    pair<::tyr::formalism::datalog::BinaryOperatorView<BooleanOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>>, bool>
    merge_p2d(BinaryOperatorView<BooleanOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeDatalogContext& context);
template std::pair<
    ::tyr::formalism::datalog::BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>>,
    bool>
merge_p2d(BinaryOperatorView<ArithmeticOperatorKind, ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeDatalogContext& context);

template std::pair<::tyr::formalism::datalog::MultiOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>>, bool>
merge_p2d(MultiOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeDatalogContext& context);
template std::pair<::tyr::formalism::datalog::MultiOperatorView<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>>, bool>
merge_p2d(MultiOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeDatalogContext& context);

template ygg::Data<::tyr::formalism::datalog::ArithmeticOperator<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>>>
merge_p2d(ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeDatalogContext& context);
template ygg::Data<::tyr::formalism::datalog::ArithmeticOperator<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>>>
merge_p2d(ArithmeticOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeDatalogContext& context);

template ygg::Data<::tyr::formalism::datalog::BooleanOperator<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::LiftedTag>>>>
merge_p2d(BooleanOperatorView<ygg::Data<FunctionExpression<::tyr::LiftedTag>>> element, MergeDatalogContext& context);
template ygg::Data<::tyr::formalism::datalog::BooleanOperator<ygg::Data<::tyr::formalism::datalog::FunctionExpression<::tyr::GroundTag>>>>
merge_p2d(BooleanOperatorView<ygg::Data<FunctionExpression<::tyr::GroundTag>>> element, MergeDatalogContext& context);

}

#endif
