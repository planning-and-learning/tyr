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

#include "tyr/planning/successors/applicability.hpp"

#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::planning
{

template ygg::float_t evaluate(ygg::float_t element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(ygg::float_t element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::UnaryOperatorView<GroundTag, formalism::ArithmeticOperatorKind> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::UnaryOperatorView<GroundTag, formalism::ArithmeticOperatorKind> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::BinaryOperatorView<GroundTag, formalism::ArithmeticOperatorKind> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::BinaryOperatorView<GroundTag, formalism::ArithmeticOperatorKind> element, const StateContext<GroundTag>& context);

template bool evaluate(formalism::BinaryOperatorView<GroundTag, formalism::BooleanOperatorKind> element, const StateContext<LiftedTag>& context);
template bool evaluate(formalism::BinaryOperatorView<GroundTag, formalism::BooleanOperatorKind> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::MultiOperatorView<GroundTag, formalism::ArithmeticOperatorKind> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::MultiOperatorView<GroundTag, formalism::ArithmeticOperatorKind> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::FunctionTermView<GroundTag, formalism::StaticTag> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::FunctionTermView<GroundTag, formalism::StaticTag> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::FunctionTermView<GroundTag, formalism::FluentTag> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::FunctionTermView<GroundTag, formalism::FluentTag> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::FunctionTermView<GroundTag, formalism::AuxiliaryTag> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::FunctionTermView<GroundTag, formalism::AuxiliaryTag> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::FunctionExpressionView<GroundTag> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::FunctionExpressionView<GroundTag> element, const StateContext<GroundTag>& context);

template bool is_applicable(formalism::BooleanOperatorView<GroundTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::BooleanOperatorView<GroundTag> element, const StateContext<GroundTag>& context);

template ygg::float_t evaluate(formalism::NumericEffectView<GroundTag, formalism::FluentTag> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::NumericEffectView<GroundTag, formalism::AuxiliaryTag> element, const StateContext<LiftedTag>& context);
template ygg::float_t evaluate(formalism::NumericEffectView<GroundTag, formalism::FluentTag> element, const StateContext<GroundTag>& context);
template ygg::float_t evaluate(formalism::NumericEffectView<GroundTag, formalism::AuxiliaryTag> element, const StateContext<GroundTag>& context);

/**
 * is_applicable_if_fires
 */

template bool is_applicable_if_fires(formalism::ConditionalEffectView<GroundTag> element,
                                     const StateContext<LiftedTag>& context,
                                     formalism::EffectFamilyList& ref_fluent_effect_families);
template bool is_applicable_if_fires(formalism::ConditionalEffectView<GroundTag> element,
                                     const StateContext<GroundTag>& context,
                                     formalism::EffectFamilyList& ref_fluent_effect_families);

template bool is_applicable_if_fires(formalism::ConditionalEffectListView<GroundTag> elements,
                                     const StateContext<LiftedTag>& context,
                                     formalism::EffectFamilyList& out_fluent_effect_families);
template bool is_applicable_if_fires(formalism::ConditionalEffectListView<GroundTag> elements,
                                     const StateContext<GroundTag>& context,
                                     formalism::EffectFamilyList& out_fluent_effect_families);

/**
 * is_applicable
 */

template bool is_applicable(formalism::LiteralView<GroundTag, formalism::StaticTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::LiteralView<GroundTag, formalism::StaticTag> element, const StateContext<GroundTag>& context);

template bool is_applicable(formalism::LiteralView<GroundTag, formalism::DerivedTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::LiteralView<GroundTag, formalism::DerivedTag> element, const StateContext<GroundTag>& context);

template bool is_applicable(formalism::LiteralListView<GroundTag, formalism::StaticTag> elements, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::LiteralListView<GroundTag, formalism::DerivedTag> elements, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::LiteralListView<GroundTag, formalism::StaticTag> elements, const StateContext<GroundTag>& context);
template bool is_applicable(formalism::LiteralListView<GroundTag, formalism::DerivedTag> elements, const StateContext<GroundTag>& context);

template bool is_applicable<formalism::PositiveTag>(formalism::FDRFactView<formalism::FluentTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable<formalism::NegativeTag>(formalism::FDRFactView<formalism::FluentTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable<formalism::PositiveTag>(formalism::FDRFactView<formalism::FluentTag> element, const StateContext<GroundTag>& context);
template bool is_applicable<formalism::NegativeTag>(formalism::FDRFactView<formalism::FluentTag> element, const StateContext<GroundTag>& context);

template bool is_applicable<formalism::PositiveTag>(formalism::FDRFactListView<formalism::FluentTag> elements, const StateContext<LiftedTag>& context);
template bool is_applicable<formalism::NegativeTag>(formalism::FDRFactListView<formalism::FluentTag> elements, const StateContext<LiftedTag>& context);
template bool is_applicable<formalism::PositiveTag>(formalism::FDRFactListView<formalism::FluentTag> elements, const StateContext<GroundTag>& context);
template bool is_applicable<formalism::NegativeTag>(formalism::FDRFactListView<formalism::FluentTag> elements, const StateContext<GroundTag>& context);

template bool is_applicable(formalism::BooleanOperatorListView<GroundTag> elements, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::BooleanOperatorListView<GroundTag> elements, const StateContext<GroundTag>& context);

template bool is_applicable(formalism::NumericEffectView<GroundTag, formalism::FluentTag> element,
                            const StateContext<LiftedTag>& context,
                            formalism::EffectFamilyList& ref_fluent_effect_families);
template bool is_applicable(formalism::NumericEffectView<GroundTag, formalism::FluentTag> element,
                            const StateContext<GroundTag>& context,
                            formalism::EffectFamilyList& ref_fluent_effect_families);

template bool is_applicable(formalism::NumericEffectOperatorListView<GroundTag, formalism::FluentTag> elements,
                            const StateContext<LiftedTag>& context,
                            formalism::EffectFamilyList& ref_fluent_effect_families);
template bool is_applicable(formalism::NumericEffectOperatorListView<GroundTag, formalism::FluentTag> elements,
                            const StateContext<GroundTag>& context,
                            formalism::EffectFamilyList& ref_fluent_effect_families);

template bool is_applicable(formalism::NumericEffectView<GroundTag, formalism::AuxiliaryTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::NumericEffectView<GroundTag, formalism::AuxiliaryTag> element, const StateContext<GroundTag>& context);

// GroundConjunctiveCondition

template bool is_applicable(formalism::ConjunctiveConditionView<GroundTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::ConjunctiveConditionView<GroundTag> element, const StateContext<GroundTag>& context);

// GroundConjunctiveEffect

template bool is_applicable(formalism::ConjunctiveEffectView<GroundTag> element,
                            const StateContext<LiftedTag>& context,
                            formalism::EffectFamilyList& ref_fluent_effect_families);
template bool is_applicable(formalism::ConjunctiveEffectView<GroundTag> element,
                            const StateContext<GroundTag>& context,
                            formalism::EffectFamilyList& ref_fluent_effect_families);

// GroundAction

template bool
is_applicable(formalism::ActionView<GroundTag> element, const StateContext<LiftedTag>& context, formalism::EffectFamilyList& out_fluent_effect_families);
template bool
is_applicable(formalism::ActionView<GroundTag> element, const StateContext<GroundTag>& context, formalism::EffectFamilyList& out_fluent_effect_families);

// GroundAxiom

template bool is_applicable(formalism::AxiomView<GroundTag> element, const StateContext<LiftedTag>& context);
template bool is_applicable(formalism::AxiomView<GroundTag> element, const StateContext<GroundTag>& context);

/**
 * is_dynamically_applicable
 */

// GroundConjunctiveCondition

template bool is_dynamically_applicable(formalism::ConjunctiveConditionView<GroundTag> element, const StateContext<LiftedTag>& context);
template bool is_dynamically_applicable(formalism::ConjunctiveConditionView<GroundTag> element, const StateContext<GroundTag>& context);

}

#endif
