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

#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_operator_utils.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/outcome_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/formalism/row_view.hpp>
//

#include "tyr/analysis/domains.hpp"
#include "tyr/datalog/declarations.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/planning/successors/action_executor.hpp"
#include "tyr/planning/successors/applicability.hpp"
#include "tyr/planning/successors/applicability_lifted.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/tasks/ground/task.hpp"
#include "tyr/planning/tasks/lifted/task.hpp"
#include "tyr/planning/states/state_builder.hpp"
#include "tyr/planning/states/state_delta.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <urd/elements/children.hpp>
#include <urd/elements/equal_to.hpp>
#include <urd/elements/flat.hpp>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

namespace
{

/// The value of `fterm` in the successor being built so far (NaN where it has none), read through its numeric storage's
/// view in the parent state's repository.
template<TaskKind Kind>
ygg::float_t value_so_far(const urd::builder<State<Kind>, StateRepository<Kind>>& successor,
                          const StateContext<Kind>& state_context,
                          urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>> fterm)
{ return NumericStorageBuilderView<Kind>(successor.numeric_storage.get(), &state_context.numeric_storage().context()).get(fterm); }

template<TaskKind Kind, typename Effects>
void process_effects(Effects effects,
                     urd::builder<State<Kind>, StateRepository<Kind>>& successor_state_builder,
                     StateContext<Kind>& state_context,
                     urd::flat_vector<urd::data<formalism::FDRFact<formalism::FluentTag>>>& tmp_del_effects,
                     urd::flat_vector<urd::data<formalism::FDRFact<formalism::FluentTag>>>& tmp_add_effects,
                     urd::Store<urd::sequence<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>>>& tmp_numeric_writes)
{
    for (const auto cond_effect : effects)
    {
        if (is_applicable(cond_effect.condition(), state_context))
        {
            const auto effect = cond_effect.effect();

            for (const auto fact : effect.template facts<formalism::NegativeTag>())
                tmp_del_effects.push_back(urd::make<formalism::FDRFact<formalism::FluentTag>>(fact.variable(), fact.value()));

            for (const auto fact : effect.template facts<formalism::PositiveTag>())
                tmp_add_effects.push_back(urd::make<formalism::FDRFact<formalism::FluentTag>>(fact.variable(), fact.value()));

            // Each operation applies to the successor's value so far, its operand read in the parent state: several
            // additive (or several multiplicative) effects on one function term combine, in any order.
            for (const auto numeric_effect : effect.numeric_effects())
            {
                const auto fterm = numeric_effect.fterm().index();
                const auto operand = evaluate(numeric_effect.fexpr(), state_context);
                successor_state_builder.set(fterm, formalism::apply(numeric_effect.operation(), value_so_far(successor_state_builder, state_context, fterm), operand));
                tmp_numeric_writes.push_back(fterm);
            }

            /// Collect the increment (total-cost) in the state_context
            if (effect.auxiliary_numeric_effect().has_value())
                state_context.auxiliary_value() = evaluate(effect.auxiliary_numeric_effect().value(), state_context);
        }
    }
}

/// Applies the effect of a firing conditional effect whose variables the grounder context's binding already extends.
void apply_lifted_effect(formalism::ConditionalEffectView<LiftedTag> cond_effect,
                         urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& successor_state_builder,
                         StateContext<LiftedTag>& state_context,
                         const ApplicabilityContext& applicability_context,
                         formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder_context,
                         formalism::FDRContext& fdr_context,
                         urd::flat_vector<urd::data<formalism::FDRFact<formalism::FluentTag>>>& tmp_del_effects,
                         urd::flat_vector<urd::data<formalism::FDRFact<formalism::FluentTag>>>& tmp_add_effects,
                         urd::Store<urd::sequence<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>>>& tmp_numeric_writes)
{
    const auto effect = cond_effect.effect();

    for (const auto literal : effect.literals())
    {
        const auto fact = fdr_context.get_fact(ground(literal.atom(), grounder_context).first);
        (literal.polarity() ? tmp_add_effects : tmp_del_effects)
            .push_back(urd::make<formalism::FDRFact<formalism::FluentTag>>(fact.variable(), fact.value()));
    }

    for (const auto numeric_effect : effect.numeric_effects())
    {
        // Sequenced: both grounding and evaluation mutate the grounder context, and argument
        // evaluation order is unspecified (gcc and clang disagree). Evaluating first preserves
        // the historical gcc (right-to-left) index assignment order. The operation applies to the successor's value so
        // far, its operand read in the parent state (as process_effects).
        const auto operand = evaluate(numeric_effect.fexpr(), applicability_context);
        const auto fterm = ground(numeric_effect.fterm(), grounder_context).first.index();
        successor_state_builder.set(fterm, formalism::apply(numeric_effect.operation(), value_so_far(successor_state_builder, state_context, fterm), operand));
        tmp_numeric_writes.push_back(fterm);
    }

    /// Collect the increment (total-cost) in the state_context
    if (effect.auxiliary_numeric_effect().has_value())
        state_context.auxiliary_value() = evaluate(effect.auxiliary_numeric_effect().value(), applicability_context);
}

/// Writes the exact changes of `successor` relative to `parent` at the touched variables and function terms into `delta`.
/// Touched entries may repeat and need not change (e.g., adding a true fact, or deleting and re-adding it).
template<TaskKind Kind>
void compute_touched_delta(const StateContext<Kind>& parent,
                           const StateContext<Kind>& successor,
                           const urd::flat_vector<urd::data<formalism::FDRFact<formalism::FluentTag>>>& del_effects,
                           const urd::flat_vector<urd::data<formalism::FDRFact<formalism::FluentTag>>>& add_effects,
                           const urd::Store<urd::sequence<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>>>& numeric_writes,
                           StateDelta& delta)
{
    delta.clear();

    for (const auto& effects : { &del_effects, &add_effects })
        for (const auto fact : *effects)
            delta.fluents.push_back(
                StateDelta::FluentChange { fact.variable, parent.fact_storage().get(fact.variable), successor.fact_storage().get(fact.variable) });
    std::ranges::sort(delta.fluents, {}, [](const auto& change) { return ygg::uint_t(change.variable.value); });
    delta.fluents.erase(std::ranges::unique(delta.fluents, {}, [](const auto& change) { return ygg::uint_t(change.variable.value); }).begin(),
                        delta.fluents.end());
    erase_if(delta.fluents, [](const auto& change) { return change.from == change.to; });

    for (const auto fterm : numeric_writes)
        delta.numerics.push_back(StateDelta::NumericChange { fterm, parent.numeric_storage().get(fterm), successor.numeric_storage().get(fterm) });
    std::ranges::sort(delta.numerics, {}, [](const auto& change) { return ygg::uint_t(change.fterm.value); });
    delta.numerics.erase(std::ranges::unique(delta.numerics, {}, [](const auto& change) { return ygg::uint_t(change.fterm.value); }).begin(),
                         delta.numerics.end());
    erase_if(delta.numerics, [](const auto& change) { return urd::EqualTo<ygg::float_t> {}(change.from, change.to); });
}

template<TaskKind Kind, typename ProcessEffects>
ygg::float_t apply_action_unregistered_impl(const StateContext<Kind>& state_context,
                                            urd::builder<State<Kind>, StateRepository<Kind>>& successor_state_builder,
                                            urd::flat_vector<urd::data<formalism::FDRFact<formalism::FluentTag>>>& del_effects,
                                            urd::flat_vector<urd::data<formalism::FDRFact<formalism::FluentTag>>>& add_effects,
                                            urd::Store<urd::sequence<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>>>& numeric_writes,
                                            StateDelta& delta,
                                            ProcessEffects process_effects)
{
    del_effects.clear();
    add_effects.clear();
    numeric_writes.clear();

    auto tmp_state_context = state_context;

    // The successor's unextended part is its parent's with the effects; its derived atoms are its axioms' to compute.
    urd::build_from(state_context.fact_storage(), *successor_state_builder.fact_storage);
    urd::build_from(state_context.numeric_storage(), *successor_state_builder.numeric_storage);
    urd::build_from(state_context.temporal_storage(), *successor_state_builder.temporal_storage);
    successor_state_builder.clear_extended_part();

    process_effects(successor_state_builder, tmp_state_context, del_effects, add_effects, numeric_writes);

    const auto& repository = state_context.fact_storage().context();
    const auto successor_facts = FactStorageBuilderView<Kind>(&*successor_state_builder.fact_storage, &repository);
    for (const auto fact : del_effects)
        if (successor_facts.get(fact.variable) == fact.value)
            successor_state_builder.set(urd::data<formalism::FDRFact<formalism::FluentTag>> { fact.variable, formalism::FDRValue::none() });

    for (const auto fact : add_effects)
        successor_state_builder.set(fact);

    compute_touched_delta(state_context,
                          StateContext<Kind> { repository, successor_state_builder, ygg::float_t { 0 } },
                          del_effects,
                          add_effects,
                          numeric_writes,
                          delta);

    return tmp_state_context.auxiliary_value();
}

}

// Ground action API

template<TaskKind Kind>
bool ActionExecutor::is_applicable(formalism::ActionView<GroundTag> action, const StateContext<Kind>& state)
{ return tyr::planning::is_applicable(action.condition(), state) && is_applicable_if_fires(action, state); }

template bool ActionExecutor::is_applicable(formalism::ActionView<GroundTag> action, const StateContext<LiftedTag>& state);
template bool ActionExecutor::is_applicable(formalism::ActionView<GroundTag> action, const StateContext<GroundTag>& state);

template<TaskKind Kind>
bool ActionExecutor::is_applicable_if_fires(formalism::ActionView<GroundTag> action, const StateContext<Kind>& state)
{ return tyr::planning::is_applicable_if_fires(action.effects(), state, m_effect_families); }

template bool ActionExecutor::is_applicable_if_fires(formalism::ActionView<GroundTag> action, const StateContext<LiftedTag>& state);
template bool ActionExecutor::is_applicable_if_fires(formalism::ActionView<GroundTag> action, const StateContext<GroundTag>& state);

template<TaskKind Kind>
ygg::float_t ActionExecutor::apply_action_unregistered(const StateContext<Kind>& state_context,
                                                       formalism::ActionView<GroundTag> action,
                                                       urd::builder<State<Kind>, StateRepository<Kind>>& successor_state_builder,
                                                       StateDelta& delta)
{
    return apply_action_unregistered_impl(state_context,
                                          successor_state_builder,
                                          m_del_effects,
                                          m_add_effects,
                                          m_numeric_writes,
                                          delta,
                                          [&](auto& out_state, auto& tmp_state_context, auto& del_effects, auto& add_effects, auto& numeric_writes)
                                          { process_effects(action.effects(), out_state, tmp_state_context, del_effects, add_effects, numeric_writes); });
}

template<TaskKind Kind>
ygg::float_t ActionExecutor::apply_effects_unregistered(const StateContext<Kind>& state_context,
                                                        std::span<const formalism::ConditionalEffectListView<GroundTag>> effects,
                                                        std::span<const formalism::OutcomeView<GroundTag>> picks,
                                                        urd::builder<State<Kind>, StateRepository<Kind>>& successor_state_builder,
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
                                              for (const auto list : effects)
                                                  process_effects(list, out_state, tmp_state_context, del_effects, add_effects, numeric_writes);
                                              for (const auto pick : picks)
                                                  process_effects(pick.effects(), out_state, tmp_state_context, del_effects, add_effects, numeric_writes);
                                          });
}

template<TaskKind Kind>
ygg::float_t ActionExecutor::apply_action_unregistered(const StateContext<Kind>& state_context,
                                                       formalism::ActionView<GroundTag> action,
                                                       std::span<const formalism::OutcomeView<GroundTag>> picks,
                                                       urd::builder<State<Kind>, StateRepository<Kind>>& successor_state_builder,
                                                       StateDelta& delta)
{
    const auto effects = std::array { action.effects() };
    return apply_effects_unregistered(state_context,
                                      std::span<const formalism::ConditionalEffectListView<GroundTag>>(effects),
                                      picks,
                                      successor_state_builder,
                                      delta);
}

template<TaskKind Kind>
bool ActionExecutor::is_applicable_if_fires(std::span<const formalism::ConditionalEffectListView<GroundTag>> effects,
                                            std::span<const formalism::OutcomeView<GroundTag>> picks,
                                            const StateContext<Kind>& state)
{
    // The families accumulate over all effects that fire.
    m_effect_families.clear();
    for (const auto list : effects)
        for (const auto cond_effect : list)
            if (!tyr::planning::is_applicable_if_fires(cond_effect, state, m_effect_families))
                return false;
    for (const auto pick : picks)
        for (const auto cond_effect : pick.effects())
            if (!tyr::planning::is_applicable_if_fires(cond_effect, state, m_effect_families))
                return false;
    return true;
}

template<TaskKind Kind>
bool ActionExecutor::is_applicable_if_fires(formalism::ActionView<GroundTag> action,
                                            std::span<const formalism::OutcomeView<GroundTag>> picks,
                                            const StateContext<Kind>& state)
{
    const auto effects = std::array { action.effects() };
    return is_applicable_if_fires(std::span<const formalism::ConditionalEffectListView<GroundTag>>(effects), picks, state);
}

template ygg::float_t ActionExecutor::apply_effects_unregistered(const StateContext<LiftedTag>& state_context,
                                                                 std::span<const formalism::ConditionalEffectListView<GroundTag>> effects,
                                                                 std::span<const formalism::OutcomeView<GroundTag>> picks,
                                                                 urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& successor_state_builder,
                                                                 StateDelta& delta);
template ygg::float_t ActionExecutor::apply_action_unregistered(const StateContext<LiftedTag>& state_context,
                                                                formalism::ActionView<GroundTag> action,
                                                                std::span<const formalism::OutcomeView<GroundTag>> picks,
                                                                urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& successor_state_builder,
                                                                StateDelta& delta);
template bool ActionExecutor::is_applicable_if_fires(std::span<const formalism::ConditionalEffectListView<GroundTag>> effects,
                                                     std::span<const formalism::OutcomeView<GroundTag>> picks,
                                                     const StateContext<LiftedTag>& state);
template bool ActionExecutor::is_applicable_if_fires(formalism::ActionView<GroundTag> action,
                                                     std::span<const formalism::OutcomeView<GroundTag>> picks,
                                                     const StateContext<LiftedTag>& state);
template ygg::float_t ActionExecutor::apply_effects_unregistered(const StateContext<GroundTag>& state_context,
                                                                 std::span<const formalism::ConditionalEffectListView<GroundTag>> effects,
                                                                 std::span<const formalism::OutcomeView<GroundTag>> picks,
                                                                 urd::builder<State<GroundTag>, StateRepository<GroundTag>>& successor_state_builder,
                                                                 StateDelta& delta);
template ygg::float_t ActionExecutor::apply_action_unregistered(const StateContext<GroundTag>& state_context,
                                                                formalism::ActionView<GroundTag> action,
                                                                std::span<const formalism::OutcomeView<GroundTag>> picks,
                                                                urd::builder<State<GroundTag>, StateRepository<GroundTag>>& successor_state_builder,
                                                                StateDelta& delta);
template bool ActionExecutor::is_applicable_if_fires(std::span<const formalism::ConditionalEffectListView<GroundTag>> effects,
                                                     std::span<const formalism::OutcomeView<GroundTag>> picks,
                                                     const StateContext<GroundTag>& state);
template bool ActionExecutor::is_applicable_if_fires(formalism::ActionView<GroundTag> action,
                                                     std::span<const formalism::OutcomeView<GroundTag>> picks,
                                                     const StateContext<GroundTag>& state);

template ygg::float_t ActionExecutor::apply_action_unregistered(const StateContext<LiftedTag>& state_context,
                                                                formalism::ActionView<GroundTag> action,
                                                                urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& successor_state_builder,
                                                                StateDelta& delta);
template ygg::float_t ActionExecutor::apply_action_unregistered(const StateContext<GroundTag>& state_context,
                                                                formalism::ActionView<GroundTag> action,
                                                                urd::builder<State<GroundTag>, StateRepository<GroundTag>>& successor_state_builder,
                                                                StateDelta& delta);

// Action binding API

bool ActionExecutor::is_applicable_if_fires(formalism::ActionView<LiftedTag> action,
                                            const StateContext<LiftedTag>& state_context,
                                            formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder,
                                            const formalism::FDRContext& fdr,
                                            ForEachEffectBinding for_each_effect_binding)
{
    auto applicability_context = ApplicabilityContext { state_context, grounder, fdr };
    m_effect_families.clear();
    auto applicable = true;
    for (const auto cond_effect : action.effects())
        for_each_effect_binding(cond_effect,
                                [&]
                                {
                                    if (applicable && !tyr::planning::is_applicable(cond_effect.effect(), applicability_context, m_effect_families))
                                        applicable = false;
                                });
    return applicable;
}

ygg::float_t ActionExecutor::apply_action_unregistered(const StateContext<LiftedTag>& state_context,
                                                       formalism::ActionView<LiftedTag> action,
                                                       formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder,
                                                       formalism::FDRContext& fdr,
                                                       ForEachEffectBinding for_each_effect_binding,
                                                       urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& successor_state_builder,
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
                                              for (const auto cond_effect : action.effects())
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

namespace
{
/// Puts `objects` into the grounder's binding.
void set_binding(formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder, std::span<const urd::index<formalism::Object>> objects)
{
    auto& binding = grounder.binding();
    binding.clear();
    for (const auto object : objects)
        binding.push_back(object);
}

/// Calls `apply(effect)` for each completed binding of `effects` (each in its action binding) and of the picked outcomes'
/// effects that extends the pick's binding.
template<typename Apply>
void for_each_bound_effect(std::span<const LiftedEffects> effects,
                           std::span<const LiftedOutcomePick> picks,
                           formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder,
                           ActionExecutor::ForEachActionEffectBinding for_each_effect_binding,
                           Apply apply)
{
    for (const auto& part : effects)
    {
        set_binding(grounder, part.binding);
        for (const auto cond_effect : part.effects)
            for_each_effect_binding(part.action, cond_effect, [&] { apply(cond_effect); });
    }
    for (const auto& pick : picks)
    {
        set_binding(grounder, pick.binding.first(pick.action.arity()));
        for (const auto cond_effect : pick.outcome.effects())
            for_each_effect_binding(pick.action,
                                    cond_effect,
                                    [&]
                                    {
                                        const auto& binding = grounder.binding();
                                        if (std::ranges::equal(std::span(binding.data(), pick.binding.size()), pick.binding))
                                            apply(cond_effect);
                                    });
    }
}
}

bool ActionExecutor::is_applicable_if_fires(std::span<const LiftedEffects> effects,
                                            std::span<const LiftedOutcomePick> picks,
                                            const StateContext<LiftedTag>& state_context,
                                            formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder,
                                            const formalism::FDRContext& fdr,
                                            ForEachActionEffectBinding for_each_effect_binding)
{
    auto applicability_context = ApplicabilityContext { state_context, grounder, fdr };
    m_effect_families.clear();
    auto applicable = true;
    for_each_bound_effect(effects,
                          picks,
                          grounder,
                          for_each_effect_binding,
                          [&](formalism::ConditionalEffectView<LiftedTag> cond_effect)
                          {
                              if (applicable && !tyr::planning::is_applicable(cond_effect.effect(), applicability_context, m_effect_families))
                                  applicable = false;
                          });
    return applicable;
}

ygg::float_t ActionExecutor::apply_effects_unregistered(const StateContext<LiftedTag>& state_context,
                                                        std::span<const LiftedEffects> effects,
                                                        std::span<const LiftedOutcomePick> picks,
                                                        formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder,
                                                        formalism::FDRContext& fdr,
                                                        ForEachActionEffectBinding for_each_effect_binding,
                                                        urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& successor_state_builder,
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
                                              for_each_bound_effect(effects,
                                                                    picks,
                                                                    grounder,
                                                                    for_each_effect_binding,
                                                                    [&](formalism::ConditionalEffectView<LiftedTag> cond_effect)
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
