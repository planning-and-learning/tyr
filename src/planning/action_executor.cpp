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

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
//

#include "tyr/analysis/domains.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/applicability_lifted.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_delta.hpp"

#include <algorithm>
#include <vector>
#include <yggdrasil/core/types.hpp>
#include <yggdrasil/semantics/equal_to.hpp>

namespace tyr::planning
{

namespace
{

template<TaskKind Kind>
void process_effects(formalism::planning::ActionView<GroundTag> action,
                     ygg::Builder<State<Kind>>& successor_state_builder,
                     StateContext<Kind>& state_context,
                     ygg::DataList<formalism::planning::FDRFact<formalism::FluentTag>>& tmp_del_effects,
                     ygg::DataList<formalism::planning::FDRFact<formalism::FluentTag>>& tmp_add_effects,
                     std::vector<ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>>>& tmp_numeric_writes)
{
    for (const auto cond_effect : action.get_effects())
    {
        if (is_applicable(cond_effect.get_condition(), state_context))
        {
            const auto effect = cond_effect.get_effect();

            for (const auto fact : effect.template get_facts<formalism::NegativeTag>())
                tmp_del_effects.push_back(fact.get_data());

            for (const auto fact : effect.template get_facts<formalism::PositiveTag>())
                tmp_add_effects.push_back(fact.get_data());

            for (const auto numeric_effect : effect.get_numeric_effects())
                visit(
                    [&](auto&& arg)
                    {
                        const auto fterm = arg.get_fterm().get_index();
                        successor_state_builder.set(fterm, evaluate(arg, state_context));
                        tmp_numeric_writes.push_back(fterm);
                    },
                    numeric_effect.get_variant());

            /// Collect the increment (total-cost) in the state_context
            if (effect.get_auxiliary_numeric_effect().has_value())
                state_context.auxiliary_value() = evaluate(effect.get_auxiliary_numeric_effect().value(), state_context);
        }
    }
}

/// Applies the effect of a firing conditional effect whose variables the grounder context's binding already extends.
void apply_lifted_effect(formalism::planning::ConditionalEffectView<LiftedTag> cond_effect,
                         ygg::Builder<State<LiftedTag>>& successor_state_builder,
                         StateContext<LiftedTag>& state_context,
                         const ApplicabilityContext& applicability_context,
                         formalism::planning::GrounderContext& grounder_context,
                         formalism::planning::FDRContext& fdr_context,
                         ygg::DataList<formalism::planning::FDRFact<formalism::FluentTag>>& tmp_del_effects,
                         ygg::DataList<formalism::planning::FDRFact<formalism::FluentTag>>& tmp_add_effects,
                         std::vector<ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>>>& tmp_numeric_writes)
{
    const auto effect = cond_effect.get_effect();

    for (const auto literal : effect.get_literals())
        if (literal.get_polarity())
            tmp_add_effects.push_back(fdr_context.get_fact(ground(literal.get_atom(), grounder_context).first).get_data());
        else
            tmp_del_effects.push_back(fdr_context.get_fact(ground(literal.get_atom(), grounder_context).first).get_data());

    for (const auto numeric_effect : effect.get_numeric_effects())
        visit(
            [&](auto&& arg)
            {
                // Sequenced: both grounding and evaluation mutate the grounder context, and argument
                // evaluation order is unspecified (gcc and clang disagree). Evaluating first preserves
                // the historical gcc (right-to-left) index assignment order.
                const auto value = evaluate(numeric_effect, applicability_context);
                const auto fterm = ground(arg.get_fterm(), grounder_context).first.get_index();
                successor_state_builder.set(fterm, value);
                tmp_numeric_writes.push_back(fterm);
            },
            numeric_effect.get_variant());

    /// Collect the increment (total-cost) in the state_context
    if (effect.get_auxiliary_numeric_effect().has_value())
        state_context.auxiliary_value() = evaluate(effect.get_auxiliary_numeric_effect().value(), applicability_context);
}

/// Writes the exact changes of `successor` relative to `parent` at the touched variables and function terms into `delta`.
/// Touched entries may repeat and need not change (e.g., adding a true fact, or deleting and re-adding it).
template<TaskKind Kind>
void compute_touched_delta(const ygg::Builder<State<Kind>>& parent,
                           const ygg::Builder<State<Kind>>& successor,
                           const ygg::DataList<formalism::planning::FDRFact<formalism::FluentTag>>& del_effects,
                           const ygg::DataList<formalism::planning::FDRFact<formalism::FluentTag>>& add_effects,
                           const std::vector<ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>>>& numeric_writes,
                           StateDelta& delta)
{
    delta.clear();

    for (const auto& effects : { &del_effects, &add_effects })
        for (const auto fact : *effects)
            delta.fluents.push_back(StateDelta::FluentChange { fact.variable, parent.get(fact.variable), successor.get(fact.variable) });
    std::ranges::sort(delta.fluents, {}, [](const auto& change) { return ygg::uint_t(change.variable); });
    delta.fluents.erase(std::ranges::unique(delta.fluents, {}, [](const auto& change) { return ygg::uint_t(change.variable); }).begin(), delta.fluents.end());
    std::erase_if(delta.fluents, [](const auto& change) { return change.from == change.to; });

    for (const auto fterm : numeric_writes)
        delta.numerics.push_back(StateDelta::NumericChange { fterm, parent.get(fterm), successor.get(fterm) });
    std::ranges::sort(delta.numerics, {}, [](const auto& change) { return ygg::uint_t(change.fterm); });
    delta.numerics.erase(std::ranges::unique(delta.numerics, {}, [](const auto& change) { return ygg::uint_t(change.fterm); }).begin(), delta.numerics.end());
    std::erase_if(delta.numerics, [](const auto& change) { return ygg::EqualTo<ygg::float_t> {}(change.from, change.to); });
}

template<TaskKind Kind, typename ProcessEffects>
ygg::float_t apply_action_unregistered_impl(const StateContext<Kind>& state_context,
                                            ygg::Builder<State<Kind>>& successor_state_builder,
                                            ygg::DataList<formalism::planning::FDRFact<formalism::FluentTag>>& del_effects,
                                            ygg::DataList<formalism::planning::FDRFact<formalism::FluentTag>>& add_effects,
                                            std::vector<ygg::Index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>>>& numeric_writes,
                                            StateDelta& delta,
                                            ProcessEffects process_effects)
{
    del_effects.clear();
    add_effects.clear();
    numeric_writes.clear();

    auto tmp_state_context = state_context;

    successor_state_builder.assign_unextended_part(tmp_state_context.state_builder());

    process_effects(successor_state_builder, tmp_state_context, del_effects, add_effects, numeric_writes);

    for (const auto fact : del_effects)
        if (successor_state_builder.get(fact.variable) == fact.value)
            successor_state_builder.set(ygg::Data<formalism::planning::FDRFact<formalism::FluentTag>> { fact.variable, formalism::planning::FDRValue::none() });

    for (const auto fact : add_effects)
        successor_state_builder.set(fact);

    compute_touched_delta(state_context.state_builder(), successor_state_builder, del_effects, add_effects, numeric_writes, delta);

    return tmp_state_context.auxiliary_value();
}

}

// Ground action API

template<TaskKind Kind>
bool ActionExecutor::is_applicable(formalism::planning::ActionView<GroundTag> action, const StateContext<Kind>& state)
{ return tyr::planning::is_applicable(action.get_condition(), state) && is_applicable_if_fires(action, state); }

template bool ActionExecutor::is_applicable(formalism::planning::ActionView<GroundTag> action, const StateContext<LiftedTag>& state);
template bool ActionExecutor::is_applicable(formalism::planning::ActionView<GroundTag> action, const StateContext<GroundTag>& state);

template<TaskKind Kind>
bool ActionExecutor::is_applicable_if_fires(formalism::planning::ActionView<GroundTag> action, const StateContext<Kind>& state)
{ return tyr::planning::is_applicable_if_fires(action.get_effects(), state, m_effect_families); }

template bool ActionExecutor::is_applicable_if_fires(formalism::planning::ActionView<GroundTag> action, const StateContext<LiftedTag>& state);
template bool ActionExecutor::is_applicable_if_fires(formalism::planning::ActionView<GroundTag> action, const StateContext<GroundTag>& state);

template<TaskKind Kind>
ygg::float_t ActionExecutor::apply_action_unregistered(const StateContext<Kind>& state_context,
                                                       formalism::planning::ActionView<GroundTag> action,
                                                       ygg::Builder<State<Kind>>& successor_state_builder,
                                                       StateDelta& delta)
{
    return apply_action_unregistered_impl(state_context,
                                          successor_state_builder,
                                          m_del_effects,
                                          m_add_effects,
                                          m_numeric_writes,
                                          delta,
                                          [&](auto& out_state, auto& tmp_state_context, auto& del_effects, auto& add_effects, auto& numeric_writes)
                                          { process_effects(action, out_state, tmp_state_context, del_effects, add_effects, numeric_writes); });
}

template ygg::float_t ActionExecutor::apply_action_unregistered(const StateContext<LiftedTag>& state_context,
                                                                formalism::planning::ActionView<GroundTag> action,
                                                                ygg::Builder<State<LiftedTag>>& successor_state_builder,
                                                                StateDelta& delta);
template ygg::float_t ActionExecutor::apply_action_unregistered(const StateContext<GroundTag>& state_context,
                                                                formalism::planning::ActionView<GroundTag> action,
                                                                ygg::Builder<State<GroundTag>>& successor_state_builder,
                                                                StateDelta& delta);

// Action binding API

bool ActionExecutor::is_applicable_if_fires(formalism::planning::ActionView<LiftedTag> action,
                                            const StateContext<LiftedTag>& state_context,
                                            formalism::planning::GrounderContext& grounder,
                                            const formalism::planning::FDRContext& fdr,
                                            ForEachEffectBinding for_each_effect_binding)
{
    auto applicability_context = ApplicabilityContext { state_context, grounder, fdr };
    m_effect_families.clear();
    auto applicable = true;
    for (const auto cond_effect : action.get_effects())
        for_each_effect_binding(cond_effect,
                                [&]
                                {
                                    if (applicable && !tyr::planning::is_applicable(cond_effect.get_effect(), applicability_context, m_effect_families))
                                        applicable = false;
                                });
    return applicable;
}

ygg::float_t ActionExecutor::apply_action_unregistered(const StateContext<LiftedTag>& state_context,
                                                       formalism::planning::ActionView<LiftedTag> action,
                                                       formalism::planning::GrounderContext& grounder,
                                                       formalism::planning::FDRContext& fdr,
                                                       ForEachEffectBinding for_each_effect_binding,
                                                       ygg::Builder<State<LiftedTag>>& successor_state_builder,
                                                       StateDelta& delta)
{
    return apply_action_unregistered_impl(state_context,
                                          successor_state_builder,
                                          m_del_effects,
                                          m_add_effects,
                                          m_numeric_writes,
                                          delta,
                                          [&](auto& out_state, auto& tmp_state_context, auto& del_effects, auto& add_effects, auto& numeric_writes)
                                          {
                                              auto applicability_context = ApplicabilityContext { tmp_state_context, grounder, fdr };
                                              for (const auto cond_effect : action.get_effects())
                                                  for_each_effect_binding(cond_effect,
                                                                          [&]
                                                                          {
                                                                              apply_lifted_effect(cond_effect,
                                                                                                  out_state,
                                                                                                  tmp_state_context,
                                                                                                  applicability_context,
                                                                                                  grounder,
                                                                                                  fdr,
                                                                                                  del_effects,
                                                                                                  add_effects,
                                                                                                  numeric_writes);
                                                                          });
                                          });
}

}
