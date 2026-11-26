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

#ifndef TYR_FORMALISM_PLANNING_GROUNDER_HPP_
#define TYR_FORMALISM_PLANNING_GROUNDER_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/grounder.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/interning.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/choice_effect_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/grounder_decl.hpp"
#include "tyr/formalism/planning/outcome_view.hpp"
#include "tyr/formalism/planning/temporal_part_view.hpp"
#include "tyr/formalism/repository.hpp"

#include <cstddef>
#include <map>
#include <optional>
#include <urd/elements/children.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>
#include <yggdrasil/formalism/declarations.hpp>
#include <yggdrasil/formalism/row_view.hpp>

/// `ground` of planning's records: facts and conditions as FDR facts, effects, actions and axioms (the shared records'
/// in formalism/grounder.hpp).
namespace tyr::formalism
{

/**
 * ground
 */

urd::data<FDRFact<FluentTag>> ground(AtomView<LiftedTag, FluentTag> element, GroundingContext<Builder, Repository>& context, FDRContext& fdr);

urd::data<FDRFact<FluentTag>> ground(LiteralView<LiftedTag, FluentTag> element, GroundingContext<Builder, Repository>& context, FDRContext& fdr);

std::pair<ConjunctiveConditionView<GroundTag>, bool>
ground(ConjunctiveConditionView<LiftedTag> element, GroundingContext<Builder, Repository>& context, FDRContext& fdr);

std::pair<ConjunctiveEffectView<GroundTag>, bool>
ground(ConjunctiveEffectView<LiftedTag> element, GroundingContext<Builder, Repository>& context, FDRContext& fdr);

std::pair<ConditionalEffectView<GroundTag>, bool>
ground(ConditionalEffectView<LiftedTag> element, GroundingContext<Builder, Repository>& context, FDRContext& fdr);

std::pair<ActionRowView, bool> ground(ActionView<LiftedTag> action, GroundingContext<Builder, Repository>& context);

/// Grounds the action binding in the context with its effects grounded for each completed binding that
/// `for_each_effect_binding(effect, callback)` puts into the context's binding before calling `callback`.
template<typename ForEachEffectBinding>
std::pair<ActionView<GroundTag>, bool>
ground(ActionView<LiftedTag> element, GroundingContext<Builder, Repository>& context, FDRContext& fdr, ForEachEffectBinding for_each_effect_binding);

std::pair<AxiomRowView, bool> ground(AxiomView<LiftedTag> axiom, GroundingContext<Builder, Repository>& context);

std::pair<AxiomView<GroundTag>, bool>
ground(AxiomView<LiftedTag> element, GroundingContext<Builder, Repository>& context, GrounderCacheEntry<Axiom<LiftedTag>>& cache, FDRContext& fdr);

/**
 * ground
 */

inline urd::data<FDRFact<FluentTag>> ground(AtomView<LiftedTag, FluentTag> element, GroundingContext<Builder, Repository>& context, FDRContext& fdr)
{
    const auto fact = fdr.get_fact(ground(element, context).first);
    return urd::make<FDRFact<FluentTag>>(fact.get_variable(), fact.get_value());
}

inline urd::data<FDRFact<FluentTag>> ground(LiteralView<LiftedTag, FluentTag> element, GroundingContext<Builder, Repository>& context, FDRContext& fdr)
{
    auto fact = ground(element.get_atom(), context, fdr);
    if (!element.get_polarity())
        fact.value = FDRValue::none();

    return fact;
}

inline std::pair<ConjunctiveConditionView<GroundTag>, bool>
ground(ConjunctiveConditionView<LiftedTag> element, GroundingContext<Builder, Repository>& context, FDRContext& fdr)
{
    // Fetch and clear
    auto conj_cond = checkout<ConjunctiveCondition<GroundTag>>(context.builder());

    // Fill data
    for (const auto literal : element.template get_literals<StaticTag>())
        urd::append(conj_cond->static_literals, ground(literal, context).first);
    for (const auto literal : element.template get_literals<FluentTag>())
    {
        if (literal.get_polarity())
            urd::build_member(conj_cond->positive_facts.emplace_back(), ground(literal.get_atom(), context, fdr), context.destination());
        else
            urd::build_member(conj_cond->negative_facts.emplace_back(), ground(literal.get_atom(), context, fdr), context.destination());
    }
    for (const auto literal : element.template get_literals<DerivedTag>())
        urd::append(conj_cond->derived_literals, ground(literal, context).first);
    for (const auto numeric_constraint : element.get_numeric_constraints())
        urd::append(conj_cond->numeric_constraints, ground(numeric_constraint, context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*conj_cond, context.builder());
}

inline std::pair<ConjunctiveEffectView<GroundTag>, bool>
ground(ConjunctiveEffectView<LiftedTag> element, GroundingContext<Builder, Repository>& context, FDRContext& fdr)
{
    // Fetch and clear
    auto conj_eff = checkout<ConjunctiveEffect<GroundTag>>(context.builder());

    for (const auto literal : element.get_literals())
    {
        const auto new_fact = ground(literal.get_atom(), context, fdr);
        if (literal.get_polarity())
            urd::build_member(conj_eff->add_facts.emplace_back(), new_fact, context.destination());
        else
            urd::build_member(conj_eff->del_facts.emplace_back(), new_fact, context.destination());
    }
    for (const auto numeric_effect : element.get_numeric_effects())
        urd::append(conj_eff->numeric_effects, ground(numeric_effect, context).first);
    if (element.get_auxiliary_numeric_effect().has_value())
        urd::store(conj_eff->auxiliary_numeric_effect, ground(element.get_auxiliary_numeric_effect().value(), context).first);

    // Canonicalize and Serialize
    return context.destination().insert(*conj_eff, context.builder());
}

inline std::pair<ConditionalEffectView<GroundTag>, bool>
ground(ConditionalEffectView<LiftedTag> element, GroundingContext<Builder, Repository>& context, FDRContext& fdr)
{
    // Fetch and clear
    auto cond_effect = checkout<ConditionalEffect<GroundTag>>(context.builder());

    // Fill data
    urd::store(cond_effect->condition, ground(element.get_condition(), context, fdr).first);
    urd::store(cond_effect->effect, ground(element.get_effect(), context, fdr).first);

    // Canonicalize and Serialize
    return context.destination().insert(*cond_effect, context.builder());
}

inline std::pair<ActionRowView, bool> ground(ActionView<LiftedTag> action, GroundingContext<Builder, Repository>& context)
{
    auto binding = checkout<ygg::Row<Action<LiftedTag>>>(context.builder());

    urd::store(binding->relation, schema_in_destination(action, context));
    for (ygg::uint_t i = 0; i < action.get_arity(); ++i)
        binding->values.push_back(context.binding()[i].value);

    // Canonicalize and Serialize
    return context.destination().insert(*binding, context.builder());
}

namespace detail
{
/// Calls `f(effect)` for each flat conditional effect of the outcome and of the outcomes of its nested choices.
template<typename F>
void for_each_outcome_effect(OutcomeView<LiftedTag> outcome, F& f)
{
    for (const auto effect : outcome.get_effects())
        f(effect);
    for (const auto choice : outcome.get_choices())
        for (const auto nested : choice.get_outcomes())
            for_each_outcome_effect(nested, f);
}

/// Grounds `choices` (an action's, or an outcome's) for the bindings that extend `enclosing` (the action binding followed by
/// the objects of the enclosing choices' variables), as `ground_choices` says, appending each ground choice to `out`. The
/// context's binding holds `enclosing` before and after.
template<typename Choices, typename ForEachEffectBinding, typename GroundCondition, typename GroundEffect, typename Out>
void ground_nested_choices(Choices choices,
                           const std::vector<urd::index<Object>>& enclosing,
                           std::size_t action_arity,
                           GroundingContext<Builder, Repository>& context,
                           ForEachEffectBinding& for_each_effect_binding,
                           GroundCondition& ground_condition,
                           GroundEffect& ground_effect,
                           Out& out)
{
    using ChoiceBinding = std::vector<urd::index<Object>>;
    using OutcomeEffects = std::vector<std::vector<ConditionalEffectView<GroundTag>>>;

    auto& binding = context.binding();
    const auto set_binding = [&](const ChoiceBinding& objects)
    {
        binding.clear();
        for (const auto object : objects)
            binding.push_back(object);
    };
    // Effect bindings are enumerated per action binding; those of other enclosing bindings are skipped.
    const auto extends_enclosing = [&]
    {
        for (std::size_t i = action_arity; i < enclosing.size(); ++i)
            if (binding[i] != enclosing[i])
                return false;
        return true;
    };
    const auto action_binding = ChoiceBinding(enclosing.begin(), enclosing.begin() + static_cast<std::ptrdiff_t>(action_arity));

    for (const auto choice : choices)
    {
        const auto choice_arity = action_arity + choice.get_arity();
        const auto outcomes = choice.get_outcomes();

        // The ground effects of each outcome per choice binding, in the order of the bindings' objects: a choice binding
        // is drawn where an effect of its outcomes (or of their nested choices) fires.
        auto groups = std::map<ChoiceBinding, OutcomeEffects> {};
        const auto group = [&]() -> OutcomeEffects&
        {
            auto prefix = ChoiceBinding();
            for (std::size_t i = 0; i < choice_arity; ++i)
                prefix.push_back(binding[i]);
            return groups.try_emplace(std::move(prefix), outcomes.size()).first->second;
        };
        for (std::size_t o = 0; o < outcomes.size(); ++o)
        {
            const auto outcome = outcomes[static_cast<std::ptrdiff_t>(o)];
            for (const auto effect : outcome.get_effects())
            {
                set_binding(action_binding);
                for_each_effect_binding(effect,
                                        [&]
                                        {
                                            if (!extends_enclosing())
                                                return;
                                            const auto ground_effect_or_nullopt = ground_effect(effect);
                                            if (ground_effect_or_nullopt)
                                                group()[o].push_back(*ground_effect_or_nullopt);
                                        });
            }
            auto register_binding = [&](ConditionalEffectView<LiftedTag> effect)
            {
                set_binding(action_binding);
                for_each_effect_binding(effect,
                                        [&]
                                        {
                                            if (extends_enclosing())
                                                static_cast<void>(group());
                                        });
            };
            for (const auto nested_choice : outcome.get_choices())
                for (const auto nested_outcome : nested_choice.get_outcomes())
                    for_each_outcome_effect(nested_outcome, register_binding);
        }

        for (const auto& [choice_binding, outcome_effects] : groups)
        {
            set_binding(choice_binding);
            const auto condition_or_nullopt = ground_condition(choice.get_condition());
            if (!condition_or_nullopt)
                continue;

            auto ground_choice = checkout<ChoiceEffect<GroundTag>>(context.builder());
            urd::store(ground_choice->condition, *condition_or_nullopt);
            auto changes = false;
            for (std::size_t o = 0; o < outcomes.size(); ++o)
            {
                auto outcome = checkout<Outcome<GroundTag>>(context.builder());
                for (const auto effect : outcome_effects[o])
                    urd::append(outcome->effects, effect);
                ground_nested_choices(outcomes[static_cast<std::ptrdiff_t>(o)].get_choices(),
                                      choice_binding,
                                      action_arity,
                                      context,
                                      for_each_effect_binding,
                                      ground_condition,
                                      ground_effect,
                                      outcome->choices);
                changes = changes || !outcome->effects.empty() || !outcome->choices.empty();
                urd::append(ground_choice->outcomes, context.destination().insert(*outcome, context.builder()).first);
            }
            if (!changes)
                continue;
            for (const auto probability : choice.get_probabilities())
                ground_choice->probabilities.push_back(probability);
            urd::append(out, context.destination().insert(*ground_choice, context.builder()).first);
        }
    }
    set_binding(enclosing);
}
}  // namespace detail

/// Grounds each of `lifted_choices` (an action's, or its end choices) in the context's binding (the action binding) for each binding of its variables for
/// which an effect of one of its outcomes (or of their nested choices) fires: the prefixes (action and choice variables)
/// of the completed bindings that `for_each_effect_binding(effect, callback)` puts into the context's binding, which holds
/// the action binding when it is called. Nested choices are grounded likewise, for the bindings that extend their
/// enclosing choice's. `ground_condition(condition)` and `ground_effect(effect)` ground in the context's binding and return
/// none for what they prune. A ground choice whose condition is pruned or whose outcomes are all empty changes nothing and
/// is dropped; empty outcomes of the others are kept (nature may pick them). Each ground choice is appended to `choices`
/// (the action builder's).
// ponytail: a nested choice re-enumerates its effects' bindings per enclosing binding (quadratic in nested fan-out); group
// once by the full prefix if nested choices ever get wide.
template<typename LiftedChoices, typename ForEachEffectBinding, typename GroundCondition, typename GroundEffect, typename Choices>
void ground_choices(LiftedChoices lifted_choices,
                    GroundingContext<Builder, Repository>& context,
                    ForEachEffectBinding for_each_effect_binding,
                    GroundCondition ground_condition,
                    GroundEffect ground_effect,
                    Choices& choices)
{
    const auto& binding = context.binding();
    const auto action_binding = std::vector<urd::index<Object>>(binding.begin(), binding.end());
    detail::ground_nested_choices(lifted_choices,
                                  action_binding,
                                  action_binding.size(),
                                  context,
                                  for_each_effect_binding,
                                  ground_condition,
                                  ground_effect,
                                  choices);
}

/// Grounds the temporal part of the action binding in the context's binding: its duration, its over-all and end
/// conditions with `ground_condition`, its end effects for each completed binding `for_each_effect_binding` puts into the
/// context's binding with `ground_effect`, and its end choices (ground_choices). None if a condition is pruned (the action
/// can never end).
template<typename ForEachEffectBinding, typename GroundCondition, typename GroundEffect>
std::optional<TemporalPartView<GroundTag>> ground_temporal_part(TemporalPartView<LiftedTag> element,
                                                                GroundingContext<Builder, Repository>& context,
                                                                ForEachEffectBinding for_each_effect_binding,
                                                                GroundCondition ground_condition,
                                                                GroundEffect ground_effect)
{
    const auto over_all = ground_condition(element.get_over_all());
    const auto end_condition = ground_condition(element.get_end_condition());
    if (!over_all || !end_condition)
        return std::nullopt;

    auto part = checkout<TemporalPart<GroundTag>>(context.builder());
    const auto ground_bounds = [&](auto lifted, auto& ground_bounds_out)
    {
        for (const auto bound : lifted)
            urd::store(ground_bounds_out.emplace_back(), ground(bound, context));
    };
    ground_bounds(element.get_min_durations(), part->min_durations);
    ground_bounds(element.get_max_durations(), part->max_durations);
    ground_bounds(element.get_end_min_durations(), part->end_min_durations);
    ground_bounds(element.get_end_max_durations(), part->end_max_durations);
    urd::store(part->over_all, *over_all);
    urd::store(part->end_condition, *end_condition);
    for (const auto effect : element.get_end_effects())
        for_each_effect_binding(effect,
                                [&]
                                {
                                    if (const auto ground_effect_or_nullopt = ground_effect(effect))
                                        urd::append(part->end_effects, *ground_effect_or_nullopt);
                                });
    ground_choices(element.get_end_choices(), context, for_each_effect_binding, ground_condition, ground_effect, part->end_choices);
    return context.destination().insert(*part, context.builder()).first;
}

template<typename ForEachEffectBinding>
std::pair<ActionView<GroundTag>, bool>
ground(ActionView<LiftedTag> element, GroundingContext<Builder, Repository>& context, FDRContext& fdr, ForEachEffectBinding for_each_effect_binding)
{
    const auto binding = ground(element, context).first.index();

    auto action = checkout<Action<GroundTag>>(context.builder());

    action->row = binding;
    urd::store(action->condition, ground(element.get_condition(), context, fdr).first);

    for (const auto cond_effect : element.get_effects())
        for_each_effect_binding(cond_effect, [&] { urd::append(action->effects, ground(cond_effect, context, fdr).first); });
    const auto ground_condition = [&](ConjunctiveConditionView<LiftedTag> condition) { return std::optional(ground(condition, context, fdr).first); };
    const auto ground_effect = [&](ConditionalEffectView<LiftedTag> effect) { return std::optional(ground(effect, context, fdr).first); };
    ground_choices(element.get_choices(), context, for_each_effect_binding, ground_condition, ground_effect, action->choices);
    if (const auto temporal_part = element.get_temporal_part(); temporal_part.has_value())
    {
        // Unpruned: its conditions always ground.
        if (const auto ground_part = ground_temporal_part(*temporal_part, context, for_each_effect_binding, ground_condition, ground_effect))
            urd::store(action->temporal_part, *ground_part);
    }

    return context.destination().insert(*action, context.builder());
}

inline std::pair<AxiomRowView, bool> ground(AxiomView<LiftedTag> axiom, GroundingContext<Builder, Repository>& context)
{
    auto binding = checkout<ygg::Row<Axiom<LiftedTag>>>(context.builder());

    urd::store(binding->relation, schema_in_destination(axiom, context));
    for (ygg::uint_t i = 0; i < axiom.get_body().get_arity(); ++i)
        binding->values.push_back(context.binding()[i].value);

    // Canonicalize and Serialize
    return context.destination().insert(*binding, context.builder());
}

inline std::pair<AxiomView<GroundTag>, bool>
ground(AxiomView<LiftedTag> element, GroundingContext<Builder, Repository>& context, GrounderCacheEntry<Axiom<LiftedTag>>& cache, FDRContext& fdr)
{
    const auto binding = ground(element, context).first.index();

    auto& axiom_cache = cache.container;
    if (auto it = axiom_cache.find(binding); it != axiom_cache.end())
        return { urd::resolve(it->second, context.destination()), false };

    auto axiom = checkout<Axiom<GroundTag>>(context.builder());

    axiom->row = binding;
    urd::store(axiom->body, ground(element.get_body(), context, fdr).first);
    urd::store(axiom->head, ground(element.get_head(), context).first);

    const auto result = context.destination().insert(*axiom, context.builder());

    axiom_cache.emplace(binding, result.first.index());

    return result;
}

}

#endif
