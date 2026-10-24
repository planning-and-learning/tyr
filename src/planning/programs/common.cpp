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

#include "common.hpp"

#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

namespace tyr::planning
{

::cista::offset::string create_applicability_name(formalism::planning::ActionView<LiftedTag> action)
{ return ::cista::offset::string { std::string { "@" } + action.get_name().str() + std::string("_") + std::to_string(action.get_index().get_value()) }; }

::cista::offset::string create_triggered_name(formalism::planning::ActionView<LiftedTag> action, formalism::planning::ConditionalEffectView<LiftedTag> cond_eff)
{ return ::cista::offset::string { create_applicability_name(action).str() + std::string("_") + std::to_string(cond_eff.get_index().get_value()) }; }

TranslationContext<LiftedTag> remap_translation_context(const TranslationContext<LiftedTag>& source, formalism::datalog::CopyContext& context)
{
    auto result = TranslationContext<LiftedTag> {};
    const auto remap_keys = [&](const auto& source_mapping, auto& result_mapping)
    {
        for (const auto& [key, value] : source_mapping)
            result_mapping.emplace(formalism::datalog::copy(key, context).first, value);
    };
    const auto remap_values = [&](const auto& source_mapping, auto& result_mapping)
    {
        for (const auto& [key, value] : source_mapping)
            result_mapping.emplace(key, formalism::datalog::copy(value, context).first);
    };

    remap_keys(source.d2p.static_to_static_predicate, result.d2p.static_to_static_predicate);
    remap_keys(source.d2p.fluent_to_fluent_predicate, result.d2p.fluent_to_fluent_predicate);
    remap_keys(source.d2p.fluent_to_derived_predicate, result.d2p.fluent_to_derived_predicate);
    remap_values(source.p2d.static_to_static_predicate, result.p2d.static_to_static_predicate);
    remap_values(source.p2d.fluent_to_fluent_predicate, result.p2d.fluent_to_fluent_predicate);
    remap_values(source.p2d.derived_to_fluent_predicate, result.p2d.derived_to_fluent_predicate);
    remap_values(source.p2d.fluent_to_fluent_function, result.p2d.fluent_to_fluent_function);
    return result;
}

::cista::offset::string create_applicability_name(formalism::planning::AxiomView<LiftedTag> axiom)
{
    return ::cista::offset::string { std::string { "@" } + axiom.get_head().get_predicate().get_name().str() + std::string("_")
                                     + std::to_string(axiom.get_index().get_value()) };
}

}
