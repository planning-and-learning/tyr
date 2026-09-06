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

#include "tyr/planning/applicability_lifted.hpp"

#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/task.hpp"

#ifndef TYR_HEADER_INSTANTIATION

namespace tyr::planning
{

// NumericEffectView

template ygg::float_t evaluate(formalism::planning::NumericEffectView<LiftedTag, formalism::FluentTag> element, const ApplicabilityContext& context);
template ygg::float_t evaluate(formalism::planning::NumericEffectView<LiftedTag, formalism::AuxiliaryTag> element, const ApplicabilityContext& context);

// NumericEffectOperatorView

template ygg::float_t evaluate(formalism::planning::NumericEffectOperatorView<LiftedTag, formalism::FluentTag> element, const ApplicabilityContext& context);
template ygg::float_t evaluate(formalism::planning::NumericEffectOperatorView<LiftedTag, formalism::AuxiliaryTag> element,
                               const ApplicabilityContext& context);

/**
 * is_applicable
 */

// LiteralListView

template bool is_applicable(formalism::planning::LiteralListView<LiftedTag, formalism::StaticTag> elements, const ApplicabilityContext& context);
template bool is_applicable(formalism::planning::LiteralListView<LiftedTag, formalism::FluentTag> elements, const ApplicabilityContext& context);
template bool is_applicable(formalism::planning::LiteralListView<LiftedTag, formalism::DerivedTag> elements, const ApplicabilityContext& context);

}

#endif
