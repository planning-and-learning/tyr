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

#ifndef TYR_PLANNING_GROUND_HEURISTICS_RPG_FF_HPP_
#define TYR_PLANNING_GROUND_HEURISTICS_RPG_FF_HPP_

#include "tyr/datalog/ground/policies/annotation.hpp"
#include "tyr/datalog/ground/policies/cost.hpp"
#include "tyr/datalog/ground/policies/numeric_support.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/planning/ground/heuristics/rpg.hpp"
#include "tyr/planning/heuristics/rpg_ff.hpp"

#include <boost/dynamic_bitset.hpp>
#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>

namespace tyr::planning
{

template<>
class FFRPGHeuristic<GroundTag> :
    public RPGBase<GroundTag,
                   FFRPGHeuristic<GroundTag>,
                   datalog::OrAnnotationPolicy<GroundTag>,
                   datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                   datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>,
                   datalog::RuleCostOverridePolicy<GroundTag>>
{
public:
    FFRPGHeuristic(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);

    static FFRPGHeuristicPtr<GroundTag> create(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL);

    ygg::float_t extract_cost_and_set_preferred_actions_impl(const StateView<GroundTag>& state);

    const ygg::UnorderedSet<::tyr::formalism::planning::GroundActionView>& get_preferred_actions() override;

    bool mark_atom(::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag> atom);
    bool mark_function(::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag> term);

private:
    void extract_relaxed_plan_and_preferred_actions(::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag> atom,
                                                    const StateContext<GroundTag>& state_context);
    void extract_relaxed_plan_and_preferred_actions(::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag> term,
                                                    const StateContext<GroundTag>& state_context);
    void extract_relaxed_plan_and_preferred_actions(::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag> term,
                                                    const datalog::Annotation<GroundTag, ::tyr::formalism::FunctionTag>& annotation,
                                                    const StateContext<GroundTag>& state_context);
    void extract_numeric_constraint_support(::tyr::formalism::datalog::GroundBooleanOperatorView constraint, const StateContext<GroundTag>& state_context);
    template<::tyr::formalism::RelationKind R>
    void extract_relaxed_plan_and_preferred_actions(const datalog::WitnessAnnotation<GroundTag, R>& witness, const StateContext<GroundTag>& state_context);

private:
    std::vector<boost::dynamic_bitset<>> m_markings;
    ygg::UnorderedSet<::tyr::formalism::datalog::GroundFunctionTermView<::tyr::formalism::FluentTag>> m_function_markings;
    datalog::GroundNumericSupportSelectorWorkspace m_numeric_support_selector_workspace;
    ::tyr::formalism::planning::EffectFamilyList m_effect_families;
    ygg::UnorderedSet<ygg::Index<::tyr::formalism::planning::GroundAction>> m_relaxed_plan;
    ygg::UnorderedSet<::tyr::formalism::planning::GroundActionView> m_preferred_actions;
};

}

#endif
