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

#ifndef TYR_PLANNING_LIFTED_HEURISTICS_RPG_FF_HPP_
#define TYR_PLANNING_LIFTED_HEURISTICS_RPG_FF_HPP_

#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/lifted/policies/numeric_support.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/formalism/datalog/formatter.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/planning/formatter.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/heuristics/rpg_ff.hpp"
#include "tyr/planning/lifted/heuristics/rpg.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>

namespace tyr::planning
{

template<>
class FFRPGHeuristic<LiftedTag> :
    public RPGBase<LiftedTag,
                   FFRPGHeuristic<LiftedTag>,
                   datalog::OrAnnotationPolicy<LiftedTag>,
                   datalog::AndAnnotationPolicy<LiftedTag, datalog::SumAggregation>,
                   datalog::TerminationPolicy<LiftedTag, datalog::SumAggregation>>
{
public:
    FFRPGHeuristic(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);

    static FFRPGHeuristicPtr<LiftedTag> create(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);

    ygg::float_t extract_cost_and_set_preferred_actions_impl(const StateView<LiftedTag>& state);

    const ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView>& get_preferred_actions() override;

    bool mark_atom(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> atom);
    bool mark_function(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> function);

private:
    void extract_relaxed_plan_and_preferred_actions(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> atom,
                                                    const StateContext<LiftedTag>& state_context,
                                                    ::tyr::formalism::planning::GrounderContext& grounder_context);
    void extract_relaxed_plan_and_preferred_actions(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> function,
                                                    const StateContext<LiftedTag>& state_context,
                                                    ::tyr::formalism::planning::GrounderContext& grounder_context);
    void extract_relaxed_plan_and_preferred_actions(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> function,
                                                    const datalog::Annotation<LiftedTag, ::tyr::formalism::FunctionTag>& annotation,
                                                    const StateContext<LiftedTag>& state_context,
                                                    ::tyr::formalism::planning::GrounderContext& grounder_context);
    void extract_numeric_constraint_support(::tyr::formalism::datalog::GroundBooleanOperatorView constraint,
                                            const StateContext<LiftedTag>& state_context,
                                            ::tyr::formalism::planning::GrounderContext& grounder_context);
    template<::tyr::formalism::RelationKind R>
    void extract_relaxed_plan_and_preferred_actions(const datalog::WitnessAnnotation<LiftedTag, R>& witness,
                                                    const StateContext<LiftedTag>& state_context,
                                                    ::tyr::formalism::planning::GrounderContext& grounder_context);

private:
    std::vector<boost::dynamic_bitset<>> m_markings;
    std::vector<boost::dynamic_bitset<>> m_function_markings;

    ygg::IndexList<::tyr::formalism::Object> m_binding;
    ActionExecutor m_executor;
    datalog::NumericSupportSelectorWorkspace<LiftedTag> m_numeric_support_selector_workspace;

    ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView> m_relaxed_plan;
    ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView> m_preferred_actions;
};

}

#endif
