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

#ifndef TYR_PLANNING_ACTION_EXECUTOR_HPP_
#define TYR_PLANNING_ACTION_EXECUTOR_HPP_

#include "tyr/declarations.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/declarations.hpp"
#include "tyr/formalism/planning/fdr_fact_data.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/outcome_view.hpp"
#include "tyr/planning/declarations.hpp"

#include <boost/compat/function_ref.hpp>
#include <cstdint>
#include <span>
#include <urd/elements/builder.hpp>
#include <urd/elements/flat.hpp>
#include <urd/elements/index.hpp>
#include <urd/kinds.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

/// An outcome of a lifted choice of `action` that nature picked, for one binding of the choice's variables: `binding` is
/// the action binding followed by the objects of the variables of the choice and of the choices it is nested in. Its
/// effects apply for the effect bindings that extend `binding`.
struct LiftedOutcomePick
{
    formalism::ActionView<LiftedTag> action;
    formalism::OutcomeView<LiftedTag> outcome;
    std::span<const urd::index<formalism::Object>> binding;
};

/// Effects of one action binding: an action's own (or those at its start), or a durative action's end effects.
struct LiftedEffects
{
    formalism::ActionView<LiftedTag> action;
    formalism::ConditionalEffectListView<LiftedTag> effects;
    std::span<const urd::index<formalism::Object>> binding;
};

class ActionExecutor
{
public:
    ActionExecutor() = default;

    // Ground action API

    template<TaskKind Kind>
    bool is_applicable(formalism::ActionView<GroundTag> action, const StateContext<Kind>& state);

    template<TaskKind Kind>
    bool is_applicable_if_fires(formalism::ActionView<GroundTag> action, const StateContext<Kind>& state);

    /// Applies the action into caller-owned storage without interning the successor and writes the exact changes relative to
    /// the parent into `delta`; returns the updated auxiliary metric value.
    template<TaskKind Kind>
    ygg::float_t apply_action_unregistered(const StateContext<Kind>& state_context,
                                           formalism::ActionView<GroundTag> action,
                                           urd::builder<State<Kind>, StateRepository<Kind>>& successor_state_builder,
                                           StateDelta& delta);

    /// Like apply_action_unregistered(), with the outcomes nature picked: the effects of each apply as the action's own
    /// do (each where its condition holds in the state).
    template<TaskKind Kind>
    ygg::float_t apply_action_unregistered(const StateContext<Kind>& state_context,
                                           formalism::ActionView<GroundTag> action,
                                           std::span<const formalism::OutcomeView<GroundTag>> picks,
                                           urd::builder<State<Kind>, StateRepository<Kind>>& successor_state_builder,
                                           StateDelta& delta);

    /// Like apply_action_unregistered(), with the effect lists given (a durative action's at its start, or the ends of the
    /// durative actions due when time advances) instead of an action's.
    template<TaskKind Kind>
    ygg::float_t apply_effects_unregistered(const StateContext<Kind>& state_context,
                                            std::span<const formalism::ConditionalEffectListView<GroundTag>> effects,
                                            std::span<const formalism::OutcomeView<GroundTag>> picks,
                                            urd::builder<State<Kind>, StateRepository<Kind>>& successor_state_builder,
                                            StateDelta& delta);

    /// Whether the numeric effects of the effect lists and of the picked outcomes that fire are valid together.
    template<TaskKind Kind>
    bool is_applicable_if_fires(std::span<const formalism::ConditionalEffectListView<GroundTag>> effects,
                                std::span<const formalism::OutcomeView<GroundTag>> picks,
                                const StateContext<Kind>& state);

    /// Whether the numeric effects of the action and of the picked outcomes that fire are valid together.
    template<TaskKind Kind>
    bool
    is_applicable_if_fires(formalism::ActionView<GroundTag> action, std::span<const formalism::OutcomeView<GroundTag>> picks, const StateContext<Kind>& state);

    // Lifted action API

    /// Calls the callback once for each completed binding of a conditional effect in the grounder's binding, which holds the action
    /// binding before and after: the action binding followed by objects of the effect's variables for which the effect fires.
    using ForEachEffectBinding = boost::compat::function_ref<void(formalism::ConditionalEffectView<LiftedTag>, boost::compat::function_ref<void()>)>;

    /// Whether the numeric effects of the action binding in `grounder` are valid together for the effect bindings that fire.
    bool is_applicable_if_fires(formalism::ActionView<LiftedTag> action,
                                const StateContext<LiftedTag>& state_context,
                                formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder,
                                const formalism::FDRContext& fdr,
                                ForEachEffectBinding for_each_effect_binding);

    /// Applies the action binding in `grounder` for the effect bindings that fire into caller-owned storage without interning the
    /// successor and writes the exact changes relative to the parent into `delta`; returns the updated auxiliary metric value.
    ygg::float_t apply_action_unregistered(const StateContext<LiftedTag>& state_context,
                                           formalism::ActionView<LiftedTag> action,
                                           formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder,
                                           formalism::FDRContext& fdr,
                                           ForEachEffectBinding for_each_effect_binding,
                                           urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& successor_state_builder,
                                           StateDelta& delta);

    /// Calls the callback once for each completed binding of an action's conditional effect, in the grounder's binding,
    /// which holds the action binding before and after.
    using ForEachActionEffectBinding =
        boost::compat::function_ref<void(formalism::ActionView<LiftedTag>, formalism::ConditionalEffectView<LiftedTag>, boost::compat::function_ref<void()>)>;

    /// Whether the numeric effects of `effects` (each in its action binding) and of the picked outcomes that fire are valid
    /// together.
    bool is_applicable_if_fires(std::span<const LiftedEffects> effects,
                                std::span<const LiftedOutcomePick> picks,
                                const StateContext<LiftedTag>& state_context,
                                formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder,
                                const formalism::FDRContext& fdr,
                                ForEachActionEffectBinding for_each_effect_binding);

    /// Applies `effects` (each in its action binding) and the picked outcomes' effects for the effect bindings that fire,
    /// into caller-owned storage without interning the successor, and writes the exact changes relative to the parent into
    /// `delta`; returns the updated auxiliary metric value. The grounder's binding is left as one of the action bindings.
    ygg::float_t apply_effects_unregistered(const StateContext<LiftedTag>& state_context,
                                            std::span<const LiftedEffects> effects,
                                            std::span<const LiftedOutcomePick> picks,
                                            formalism::GroundingContext<formalism::Builder, formalism::Repository>& grounder,
                                            formalism::FDRContext& fdr,
                                            ForEachActionEffectBinding for_each_effect_binding,
                                            urd::builder<State<LiftedTag>, StateRepository<LiftedTag>>& successor_state_builder,
                                            StateDelta& delta);

    /// Appends the FDR variables and fluent function terms the last application wrote (its fired effects).
    template<typename Variables, typename Terms>
    void fired_writes(Variables& variables, Terms& terms) const
    {
        for (const auto& effects : { &m_del_effects, &m_add_effects })
            for (const auto fact : *effects)
                variables.push_back(static_cast<std::uint32_t>(fact.variable.value));
        for (const auto term : m_numeric_writes)
            terms.push_back(static_cast<std::uint32_t>(term.value));
    }

private:
    urd::flat_vector<urd::data<formalism::FDRFact<formalism::FluentTag>>> m_del_effects;
    urd::flat_vector<urd::data<formalism::FDRFact<formalism::FluentTag>>> m_add_effects;
    urd::Store<urd::sequence<urd::index<formalism::FunctionTerm<GroundTag, formalism::FluentTag>>>> m_numeric_writes;
    formalism::EffectFamilyList m_effect_families;
};
}

#endif
