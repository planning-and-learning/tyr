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
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_context.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_value.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/grounder_decl.hpp"
#include "tyr/formalism/repository.hpp"

#include <urd/elements/children.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/view.hpp>
#include <utility>
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
