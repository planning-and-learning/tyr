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

#include "tyr/planning/applicability.hpp"

#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/task.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::planning
{

template ygg::float_t evaluate(ygg::float_t element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(ygg::float_t element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::planning::UnaryOperatorView<GroundTag> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::planning::UnaryOperatorView<GroundTag> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::planning::BinaryOperatorView<GroundTag, formalism::ArithmeticOperatorKind> element,
                               const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::planning::BinaryOperatorView<GroundTag, formalism::ArithmeticOperatorKind> element,
                               const StateContext<GroundTag>& context);

template bool evaluate(formalism::planning::BinaryOperatorView<GroundTag, formalism::BooleanOperatorKind> element,
                       const StateContext<LiftedTag>& context);
template bool evaluate(formalism::planning::BinaryOperatorView<GroundTag, formalism::BooleanOperatorKind> element,
                       const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::planning::MultiOperatorView<GroundTag> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::planning::MultiOperatorView<GroundTag> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::planning::FunctionTermView<GroundTag, formalism::StaticTag> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::planning::FunctionTermView<GroundTag, formalism::StaticTag> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::planning::FunctionTermView<GroundTag, formalism::FluentTag> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::planning::FunctionTermView<GroundTag, formalism::FluentTag> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::planning::FunctionTermView<GroundTag, formalism::AuxiliaryTag> element,
                               const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::planning::FunctionTermView<GroundTag, formalism::AuxiliaryTag> element,
                               const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::planning::FunctionExpressionView<GroundTag> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::planning::FunctionExpressionView<GroundTag> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::planning::ArithmeticOperatorView<GroundTag> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::planning::ArithmeticOperatorView<GroundTag> element, const StateContext<GroundTag>& context);

template bool is_applicable(formalism::planning::BooleanOperatorView<GroundTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::planning::BooleanOperatorView<GroundTag> element, const StateContext<GroundTag>& context);

template bool evaluate(formalism::planning::BooleanOperatorView<GroundTag> element, const StateContext<LiftedTag>& context);
template bool evaluate(formalism::planning::BooleanOperatorView<GroundTag> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::planning::NumericEffectView<GroundTag, formalism::FluentTag> element,
                               const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::planning::NumericEffectView<GroundTag, formalism::AuxiliaryTag> element,
                               const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::planning::NumericEffectView<GroundTag, formalism::FluentTag> element,
                               const StateContext<GroundTag>& context);
template ygg::float_t evaluate(formalism::planning::NumericEffectView<GroundTag, formalism::AuxiliaryTag> element,
                               const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::planning::NumericEffectOperatorView<GroundTag, formalism::FluentTag> element,
                               const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::planning::NumericEffectOperatorView<GroundTag, formalism::AuxiliaryTag> element,
                               const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::planning::NumericEffectOperatorView<GroundTag, formalism::FluentTag> element,
                               const StateContext<GroundTag>& context);
template ygg::float_t evaluate(formalism::planning::NumericEffectOperatorView<GroundTag, formalism::AuxiliaryTag> element,
                               const StateContext<GroundTag>& context);

/**
 * is_applicable_if_fires
 */

template bool is_applicable_if_fires(formalism::planning::ConditionalEffectView<GroundTag> element,
                                     const StateContext<LiftedTag>& context,
                                     formalism::planning::EffectFamilyList& ref_fluent_effect_families);
template bool is_applicable_if_fires(formalism::planning::ConditionalEffectView<GroundTag> element,
                                     const StateContext<GroundTag>& context,
                                     formalism::planning::EffectFamilyList& ref_fluent_effect_families);

template bool is_applicable_if_fires(formalism::planning::ConditionalEffectListView<GroundTag> elements,
                                     const StateContext<LiftedTag>& context,
                                     formalism::planning::EffectFamilyList& out_fluent_effect_families);
template bool is_applicable_if_fires(formalism::planning::ConditionalEffectListView<GroundTag> elements,
                                     const StateContext<GroundTag>& context,
                                     formalism::planning::EffectFamilyList& out_fluent_effect_families);

/**
 * is_applicable
 */

template bool is_applicable(formalism::planning::LiteralView<GroundTag, formalism::StaticTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::planning::LiteralView<GroundTag, formalism::StaticTag> element, const StateContext<GroundTag>& context);

template bool is_applicable(formalism::planning::LiteralView<GroundTag, formalism::DerivedTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::planning::LiteralView<GroundTag, formalism::DerivedTag> element, const StateContext<GroundTag>& context);

template bool is_applicable(formalism::planning::LiteralListView<GroundTag, formalism::StaticTag> elements, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::planning::LiteralListView<GroundTag, formalism::DerivedTag> elements, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::planning::LiteralListView<GroundTag, formalism::StaticTag> elements, const StateContext<GroundTag>& context);
template bool is_applicable(formalism::planning::LiteralListView<GroundTag, formalism::DerivedTag> elements, const StateContext<GroundTag>& context);

template bool is_applicable<formalism::PositiveTag>(formalism::planning::FDRFactView<formalism::FluentTag> element,
                                                           const StateContext<LiftedTag>& context);
template bool is_applicable<formalism::NegativeTag>(formalism::planning::FDRFactView<formalism::FluentTag> element,
                                                           const StateContext<LiftedTag>& context);
template bool is_applicable<formalism::PositiveTag>(formalism::planning::FDRFactView<formalism::FluentTag> element,
                                                           const StateContext<GroundTag>& context);
template bool is_applicable<formalism::NegativeTag>(formalism::planning::FDRFactView<formalism::FluentTag> element,
                                                           const StateContext<GroundTag>& context);

template bool is_applicable<formalism::PositiveTag>(formalism::planning::FDRFactListView<formalism::FluentTag> elements,
                                                           const StateContext<LiftedTag>& context);
template bool is_applicable<formalism::NegativeTag>(formalism::planning::FDRFactListView<formalism::FluentTag> elements,
                                                           const StateContext<LiftedTag>& context);
template bool is_applicable<formalism::PositiveTag>(formalism::planning::FDRFactListView<formalism::FluentTag> elements,
                                                           const StateContext<GroundTag>& context);
template bool is_applicable<formalism::NegativeTag>(formalism::planning::FDRFactListView<formalism::FluentTag> elements,
                                                           const StateContext<GroundTag>& context);

template bool is_applicable(formalism::planning::BooleanOperatorListView<GroundTag> elements, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::planning::BooleanOperatorListView<GroundTag> elements, const StateContext<GroundTag>& context);

template bool is_applicable(formalism::planning::NumericEffectView<GroundTag, formalism::FluentTag> element,
                            const StateContext<LiftedTag>& context,
                            formalism::planning::EffectFamilyList& ref_fluent_effect_families);
template bool is_applicable(formalism::planning::NumericEffectView<GroundTag, formalism::FluentTag> element,
                            const StateContext<GroundTag>& context,
                            formalism::planning::EffectFamilyList& ref_fluent_effect_families);

template bool is_applicable(formalism::planning::NumericEffectOperatorView<GroundTag, formalism::FluentTag> element,
                            const StateContext<LiftedTag>& context,
                            formalism::planning::EffectFamilyList& ref_fluent_effect_families);
template bool is_applicable(formalism::planning::NumericEffectOperatorView<GroundTag, formalism::FluentTag> element,
                            const StateContext<GroundTag>& context,
                            formalism::planning::EffectFamilyList& ref_fluent_effect_families);

template bool is_applicable(formalism::planning::NumericEffectOperatorListView<GroundTag, formalism::FluentTag> elements,
                            const StateContext<LiftedTag>& context,
                            formalism::planning::EffectFamilyList& ref_fluent_effect_families);
template bool is_applicable(formalism::planning::NumericEffectOperatorListView<GroundTag, formalism::FluentTag> elements,
                            const StateContext<GroundTag>& context,
                            formalism::planning::EffectFamilyList& ref_fluent_effect_families);

template bool is_applicable(formalism::planning::NumericEffectView<GroundTag, formalism::AuxiliaryTag> element,
                            const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::planning::NumericEffectView<GroundTag, formalism::AuxiliaryTag> element,
                            const StateContext<GroundTag>& context);

template bool is_applicable(formalism::planning::NumericEffectOperatorView<GroundTag, formalism::AuxiliaryTag> element,
                            const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::planning::NumericEffectOperatorView<GroundTag, formalism::AuxiliaryTag> element,
                            const StateContext<GroundTag>& context);

// GroundConjunctiveCondition

template bool is_applicable(formalism::planning::ConjunctiveConditionView<GroundTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::planning::ConjunctiveConditionView<GroundTag> element, const StateContext<GroundTag>& context);

// GroundConjunctiveEffect

template bool is_applicable(formalism::planning::ConjunctiveEffectView<GroundTag> element,
                            const StateContext<LiftedTag>& context,
                            formalism::planning::EffectFamilyList& ref_fluent_effect_families);
template bool is_applicable(formalism::planning::ConjunctiveEffectView<GroundTag> element,
                            const StateContext<GroundTag>& context,
                            formalism::planning::EffectFamilyList& ref_fluent_effect_families);

// GroundAction

template bool is_applicable(formalism::planning::ActionView<GroundTag> element,
                            const StateContext<LiftedTag>& context,
                            formalism::planning::EffectFamilyList& out_fluent_effect_families);
template bool is_applicable(formalism::planning::ActionView<GroundTag> element,
                            const StateContext<GroundTag>& context,
                            formalism::planning::EffectFamilyList& out_fluent_effect_families);

// GroundAxiom

template bool is_applicable(formalism::planning::AxiomView<GroundTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::planning::AxiomView<GroundTag> element, const StateContext<GroundTag>& context);

/**
 * is_dynamically_applicable
 */

// GroundConjunctiveCondition

template bool is_dynamically_applicable(formalism::planning::ConjunctiveConditionView<GroundTag> element, const StateContext<LiftedTag>& context);
template bool is_dynamically_applicable(formalism::planning::ConjunctiveConditionView<GroundTag> element, const StateContext<GroundTag>& context);

}

#endif
