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

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/formalism/datalog/copy.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"

#include <cassert>
#include <vector>

namespace tyr::planning
{

::cista::offset::string create_applicability_name(formalism::planning::ActionView<LiftedTag> action)
{
    return ::cista::offset::string { std::string { "@" } + action.get_name().str() + std::string("_") + std::to_string(action.get_index().get_value()) };
}

::cista::offset::string create_triggered_name(formalism::planning::ActionView<LiftedTag> action, formalism::planning::ConditionalEffectView<LiftedTag> cond_eff)
{
    return ::cista::offset::string { create_applicability_name(action).str() + std::string("_") + std::to_string(cond_eff.get_index().get_value()) };
}

bool has_effect_rule(formalism::planning::ConditionalEffectView<LiftedTag> effect)
{
    const auto condition = effect.get_condition();
    return !effect.get_variables().empty() || !condition.get_literals<formalism::StaticTag>().empty()
           || !condition.get_literals<formalism::FluentTag>().empty() || !condition.get_literals<formalism::DerivedTag>().empty()
           || !condition.get_numeric_constraints().empty();
}

EffectAtoms group_effect_atoms(formalism::planning::ActionView<LiftedTag> action,
                               std::span<const ygg::Index<formalism::datalog::Rule<LiftedTag, formalism::PredicateTag>>> effect_rules,
                               const formalism::datalog::Repository& repository,
                               const datalog::PredicateFactSets<formalism::FluentTag>& facts)
{
    auto result = EffectAtoms {};
#ifndef NDEBUG
    // The k-th effect rule belongs to the k-th effect with one; its last head has the action's variables followed by the effect's.
    auto effect_arities = std::vector<size_t> {};
    for (const auto effect : action.get_effects())
        if (has_effect_rule(effect))
            effect_arities.push_back(effect.get_arity());
    assert(effect_arities.size() == effect_rules.size());
#endif
    for (size_t k = 0; k < effect_rules.size(); ++k)
    {
        const auto predicate = ygg::make_view(effect_rules[k], repository).get_heads().back().get_predicate();
        assert(predicate.get_arity() == action.get_arity() + effect_arities[k]);
        for (const auto atom : facts.get_sets()[ygg::uint_t(predicate.get_index())].get_bindings())
        {
            const auto objects = atom.get_objects();
            auto binding = ygg::IndexList<formalism::Object> {};
            auto extension = ygg::IndexList<formalism::Object> {};
            for (size_t i = 0; i < objects.size(); ++i)
                (i < action.get_arity() ? binding : extension).push_back(objects[i].get_index());
            result[std::move(binding)].emplace_back(k, std::move(extension));
        }
    }
    return result;
}

void collect_firing_effects(formalism::planning::ActionView<LiftedTag> action,
                            std::span<const std::pair<size_t, ygg::IndexList<formalism::Object>>> atoms,
                            std::vector<ActionExecutor::FiringEffect>& out)
{
    out.clear();
    auto k = size_t(0);
    for (const auto effect : action.get_effects())
    {
        if (!has_effect_rule(effect))
        {
            out.push_back({ effect, {} });
            continue;
        }
        for (const auto& [atom_k, extension] : atoms)
            if (atom_k == k)
                out.push_back({ effect, std::span<const ygg::Index<formalism::Object>>(extension.data(), extension.size()) });
        ++k;
    }
}

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
