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

#include "tyr/datalog/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/applicability.hpp"
#include "tyr/planning/applicability_lifted.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/node.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/state_repository.hpp"
#include "tyr/planning/ground/state_view.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/lifted/state_builder.hpp"
#include "tyr/planning/lifted/state_repository.hpp"
#include "tyr/planning/lifted/state_view.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <yggdrasil/core/types.hpp>

namespace f = tyr::formalism;
namespace fp = tyr::formalism::planning;

namespace tyr::planning
{

template<TaskKind Kind>
void process_effects(fp::GroundActionView action,
                     UnpackedState<Kind>& succ_unpacked_state,
                     StateContext<Kind>& state_context,
                     ygg::DataList<fp::FDRFact<f::FluentTag>>& tmp_del_effects,
                     ygg::DataList<fp::FDRFact<f::FluentTag>>& tmp_add_effects)
{
    for (const auto cond_effect : action.get_effects())
    {
        if (is_applicable(cond_effect.get_condition(), state_context))
        {
            const auto effect = cond_effect.get_effect();

            for (const auto fact : effect.template get_facts<f::NegativeTag>())
                tmp_del_effects.push_back(fact.get_data());

            for (const auto fact : effect.template get_facts<f::PositiveTag>())
                tmp_add_effects.push_back(fact.get_data());

            for (const auto numeric_effect : effect.get_numeric_effects())
                visit([&](auto&& arg) { succ_unpacked_state.set(arg.get_fterm().get_index(), evaluate(numeric_effect, state_context)); },
                      numeric_effect.get_variant());

            /// Collect the increment (total-cost) in the state_context
            if (effect.get_auxiliary_numeric_effect().has_value())
                state_context.auxiliary_value = evaluate(effect.get_auxiliary_numeric_effect().value(), state_context);
        }
    }
}

inline void process_effects(fp::ActionView action,
                            const analysis::ActionDomain& action_domain,
                            UnpackedState<LiftedTag>& succ_unpacked_state,
                            StateContext<LiftedTag>& state_context,
                            fp::GrounderContext& grounder_context,
                            fp::FDRContext& fdr_context,
                            ygg::itertools::cartesian_set::Workspace<ygg::Index<f::Object>>& cartesian_workspace,
                            ::tyr::formalism::planning::EffectFamilyList& effect_families,
                            ygg::DataList<fp::FDRFact<f::FluentTag>>& tmp_del_effects,
                            ygg::DataList<fp::FDRFact<f::FluentTag>>& tmp_add_effects)
{
    auto applicability_context = ApplicabilityContext { state_context, grounder_context, fdr_context };

    for (const auto cond_effect : action.get_effects())
    {
        const auto& parameter_domains = action_domain.payload.effect_domains.at(cond_effect.get_index()).payload.effect_domain.payload;
        const auto binding_size = grounder_context.binding.size();

        ygg::itertools::cartesian_set::for_each_element(
            parameter_domains.begin() + action.get_arity(),
            parameter_domains.end(),
            cartesian_workspace,
            [&](auto&& binding_cond)
            {
                grounder_context.binding.resize(binding_size);
                grounder_context.binding.insert(grounder_context.binding.end(), binding_cond.begin(), binding_cond.end());

                if (is_applicable(cond_effect.get_condition(), applicability_context))
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
                                succ_unpacked_state.set(ground(arg.get_fterm(), grounder_context).first.get_index(), value);
                            },
                            numeric_effect.get_variant());

                    /// Collect the increment (total-cost) in the state_context
                    if (effect.get_auxiliary_numeric_effect().has_value())
                        state_context.auxiliary_value = evaluate(effect.get_auxiliary_numeric_effect().value(), applicability_context);
                }

                grounder_context.binding.resize(binding_size);
            });
    }
}

template<TaskKind Kind, typename ProcessEffects>
Node<Kind> apply_action_impl(const StateContext<Kind>& state_context,
                             StateRepository<Kind>& state_repository,
                             ygg::DataList<fp::FDRFact<f::FluentTag>>& del_effects,
                             ygg::DataList<fp::FDRFact<f::FluentTag>>& add_effects,
                             ProcessEffects&& process_effects)
{
    del_effects.clear();
    add_effects.clear();

    auto tmp_state_context = state_context;
    auto& task = tmp_state_context.task;

    auto succ_unpacked_state_ptr = state_repository.get_unregistered_state();
    auto& succ_unpacked_state = *succ_unpacked_state_ptr;
    succ_unpacked_state.assign_unextended_part(tmp_state_context.unpacked_state);

    process_effects(succ_unpacked_state, tmp_state_context, del_effects, add_effects);

    for (const auto fact : del_effects)
        if (succ_unpacked_state.get(fact.variable) == fact.value)
            succ_unpacked_state.set(ygg::Data<fp::FDRFact<f::FluentTag>> { fact.variable, fp::FDRValue::none() });

    for (const auto fact : add_effects)
        succ_unpacked_state.set(fact);

    auto succ_state = state_repository.register_state(succ_unpacked_state_ptr);

    auto succ_state_context = StateContext { task, succ_unpacked_state, tmp_state_context.auxiliary_value };
    if (task.get_task().get_metric())
        succ_state_context.auxiliary_value = evaluate(task.get_task().get_metric().value().get_fexpr(), succ_state_context);
    else
        ++succ_state_context.auxiliary_value;  // Assume unit cost if no metric is given

    return Node<Kind>(succ_state, ygg::FloatTolerance<ygg::float_t>::canonicalize(succ_state_context.auxiliary_value));
}

// Ground action API

template<TaskKind Kind>
bool ActionExecutor::is_applicable(fp::GroundActionView action, const StateContext<Kind>& state)
{
    return tyr::planning::is_applicable(action.get_condition(), state) && is_applicable_if_fires(action, state);
}

template bool ActionExecutor::is_applicable(fp::GroundActionView action, const StateContext<LiftedTag>& state);
template bool ActionExecutor::is_applicable(fp::GroundActionView action, const StateContext<GroundTag>& state);

template<TaskKind Kind>
bool ActionExecutor::is_applicable_if_fires(fp::GroundActionView action, const StateContext<Kind>& state)
{
    return tyr::planning::is_applicable_if_fires(action.get_effects(), state, m_effect_families);
}

template bool ActionExecutor::is_applicable_if_fires(fp::GroundActionView action, const StateContext<LiftedTag>& state);
template bool ActionExecutor::is_applicable_if_fires(fp::GroundActionView action, const StateContext<GroundTag>& state);

template<TaskKind Kind>
Node<Kind> ActionExecutor::apply_action(const StateContext<Kind>& state_context, fp::GroundActionView action, StateRepository<Kind>& state_repository)
{
    return apply_action_impl(state_context,
                             state_repository,
                             m_del_effects,
                             m_add_effects,
                             [&](auto& succ_unpacked_state, auto& tmp_state_context, auto& del_effects, auto& add_effects)
                             { process_effects(action, succ_unpacked_state, tmp_state_context, del_effects, add_effects); });
}

template Node<LiftedTag>
ActionExecutor::apply_action(const StateContext<LiftedTag>& state_context, fp::GroundActionView action, StateRepository<LiftedTag>& state_repository);
template Node<GroundTag>
ActionExecutor::apply_action(const StateContext<GroundTag>& state_context, fp::GroundActionView action, StateRepository<GroundTag>& state_repository);

// Action binding API

bool ActionExecutor::is_applicable(fp::ActionView action,
                                   const StateContext<LiftedTag>& state_context,
                                   fp::GrounderContext& grounder,
                                   const fp::FDRContext& fdr)
{
    auto applicability_context = ApplicabilityContext { state_context, grounder, fdr };

    return tyr::planning::is_applicable(action.get_condition(), applicability_context) && is_applicable_if_fires(action, state_context, grounder, fdr);
}

bool ActionExecutor::is_applicable_if_fires(fp::ActionView action,
                                            const StateContext<LiftedTag>& state_context,
                                            fp::GrounderContext& grounder,
                                            const fp::FDRContext& fdr)
{
    auto applicability_context = ApplicabilityContext { state_context, grounder, fdr };

    return tyr::planning::is_applicable_if_fires(action.get_effects(),
                                                 applicability_context,
                                                 m_effect_families,
                                                 m_cartesian_workspace,
                                                 state_context.task.get_formalism_task().get_variable_domains().action_domains.at(action.get_index()));
}

Node<LiftedTag> ActionExecutor::apply_action(const StateContext<LiftedTag>& state_context,
                                             fp::ActionView action,
                                             fp::GrounderContext& grounder,
                                             fp::FDRContext& fdr,
                                             StateRepository<LiftedTag>& state_repository)
{
    return apply_action_impl(state_context,
                             state_repository,
                             m_del_effects,
                             m_add_effects,
                             [&](auto& succ_unpacked_state, auto& tmp_state_context, auto& del_effects, auto& add_effects)
                             {
                                 process_effects(action,
                                                 state_context.task.get_formalism_task().get_variable_domains().action_domains.at(action.get_index()),
                                                 succ_unpacked_state,
                                                 tmp_state_context,
                                                 grounder,
                                                 fdr,
                                                 m_cartesian_workspace,
                                                 m_effect_families,
                                                 del_effects,
                                                 add_effects);
                             });
}
}
