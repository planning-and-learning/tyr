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

#include "tyr/formalism/planning/declarations.hpp"
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/binary_operator_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/function_term_value_view.hpp"
#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/literal_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/multi_operator_view.hpp"
#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/planning/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/applicability_lifted_decl.hpp"
#include "tyr/planning/declarations.hpp"

#include <boost/compat/function_ref.hpp>
#include <span>
#include <vector>
#include <urd/elements/flat.hpp>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

class ActionExecutor
{
public:
    ActionExecutor() = default;

    // Ground action API

    template<TaskKind Kind>
    bool is_applicable(formalism::planning::ActionView<GroundTag> action, const StateContext<Kind>& state);

    template<TaskKind Kind>
    bool is_applicable_if_fires(formalism::planning::ActionView<GroundTag> action, const StateContext<Kind>& state);

    /// Applies the action into caller-owned storage without interning the successor and writes the exact changes relative to
    /// the parent into `delta`; returns the updated auxiliary metric value.
    template<TaskKind Kind>
    ygg::float_t apply_action_unregistered(const StateContext<Kind>& state_context,
                                           formalism::planning::ActionView<GroundTag> action,
                                           urd::builder<State<Kind>>& successor_state_builder,
                                           StateDelta& delta);

    // Lifted action API

    /// Calls the callback once for each completed binding of a conditional effect in the grounder's binding, which holds the action
    /// binding before and after: the action binding followed by objects of the effect's variables for which the effect fires.
    using ForEachEffectBinding = boost::compat::function_ref<void(formalism::planning::ConditionalEffectView<LiftedTag>, boost::compat::function_ref<void()>)>;

    /// Whether the numeric effects of the action binding in `grounder` are valid together for the effect bindings that fire.
    bool is_applicable_if_fires(formalism::planning::ActionView<LiftedTag> action,
                                const StateContext<LiftedTag>& state_context,
                                formalism::WriteContext<formalism::planning::Builder, formalism::planning::Repository>& grounder,
                                const formalism::planning::FDRContext& fdr,
                                ForEachEffectBinding for_each_effect_binding);

    /// Applies the action binding in `grounder` for the effect bindings that fire into caller-owned storage without interning the
    /// successor and writes the exact changes relative to the parent into `delta`; returns the updated auxiliary metric value.
    ygg::float_t apply_action_unregistered(const StateContext<LiftedTag>& state_context,
                                           formalism::planning::ActionView<LiftedTag> action,
                                           formalism::WriteContext<formalism::planning::Builder, formalism::planning::Repository>& grounder,
                                           formalism::planning::FDRContext& fdr,
                                           ForEachEffectBinding for_each_effect_binding,
                                           urd::builder<State<LiftedTag>>& successor_state_builder,
                                           StateDelta& delta);

private:
    urd::flat_vector<urd::data<formalism::planning::FDRFact<formalism::FluentTag>>> m_del_effects;
    urd::flat_vector<urd::data<formalism::planning::FDRFact<formalism::FluentTag>>> m_add_effects;
    urd::Store<urd::sequence<urd::index<formalism::planning::FunctionTerm<GroundTag, formalism::FluentTag>>>> m_numeric_writes;
    formalism::planning::EffectFamilyList m_effect_families;
};
}

#endif
