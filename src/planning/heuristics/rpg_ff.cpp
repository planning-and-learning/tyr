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

#include "tyr/planning/heuristics/rpg_ff.hpp"

#include "../ground/heuristics/rpg.hpp"
#include "../lifted/heuristics/rpg.hpp"
#include "rpg.hpp"
#include "tyr/datalog/policies/annotation.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/numeric_support.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/planning/action_executor.hpp"

#include <cassert>
#include <deque>
#include <utility>
#include <vector>
#include <yggdrasil/containers/dynamic_bitset.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
struct FFRPGHeuristic<Kind>::Impl :
    detail::RPGEvaluator<Impl, Kind, datalog::MinCostAnnotationPolicy<Kind, datalog::SumAggregation>, datalog::TerminationPolicy<Kind, datalog::SumAggregation>>
{
    using Base = detail::
        RPGEvaluator<Impl, Kind, datalog::MinCostAnnotationPolicy<Kind, datalog::SumAggregation>, datalog::TerminationPolicy<Kind, datalog::SumAggregation>>;
    using PredicateHead = ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag>;
    using FunctionHead = ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag>;
    using NumericAnnotation = datalog::Annotation<Kind, ::tyr::formalism::FunctionTag>;
    using NumericSupportWorkspace = datalog::NumericSupportSelectorWorkspace<Kind>;

    Impl(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
        Base(std::move(task), std::move(execution_context), cost_mode),
        m_predicate_markings(this->m_workspace.facts.fact_sets.predicate.get_sets().size()),
        m_function_markings(this->m_workspace.facts.fact_sets.function.get_sets().size())
    {
    }

    Impl(const Impl& source, ygg::ExecutionContextPtr execution_context) :
        Base(source, std::move(execution_context)),
        m_predicate_markings(this->m_workspace.facts.fact_sets.predicate.get_sets().size()),
        m_function_markings(this->m_workspace.facts.fact_sets.function.get_sets().size())
    {
    }

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal)
    {
        Base::set_goal(goal);
        m_relaxed_plan.clear();
        m_preferred_actions.clear();
    }

    ygg::float_t evaluate(const ygg::Builder<State<Kind>>& state)
    {
        for (auto& workspace : m_numeric_support_workspaces)
            workspace.clear();
        m_relaxed_plan.clear();
        m_preferred_actions.clear();
        return Base::evaluate(state);
    }

    ygg::float_t compute_result(const ygg::Builder<State<Kind>>& state)
    {
        for (auto& bits : m_predicate_markings)
            bits.reset();
        for (auto& bits : m_function_markings)
            bits.reset();

        const auto state_context = StateContext<Kind>(this->get_task(), state, ygg::float_t(0));
        if (const auto& goal = this->m_workspace.tp.get_goal())
        {
            for (const auto literal : goal->template get_literals<::tyr::formalism::FluentTag>())
            {
                assert(literal.get_polarity());
                extract_relaxed_plan(literal.get_atom().get_row(), state_context, 0);
            }

            for (const auto constraint : goal->get_numeric_constraints())
                extract_numeric_constraint_support(constraint, state_context, 0);
        }

        return ygg::float_t(m_relaxed_plan.size());
    }

    const auto& get_preferred_actions() const noexcept { return m_preferred_actions; }

private:
    void extract_relaxed_plan(PredicateHead head, const StateContext<Kind>& state_context, size_t numeric_support_depth)
    {
        if (mark(m_predicate_markings, head.get_index().relation, head.get_index().row))
            return;

        const auto* annotation = this->m_workspace.annotations.find(head);
        if (!annotation)
            return;

        const auto* witness = std::get_if<datalog::WitnessAnnotation<Kind>>(annotation);
        if (witness)
            extract_relaxed_plan(*witness, state_context, numeric_support_depth);
    }

    void extract_relaxed_plan(FunctionHead head, const StateContext<Kind>& state_context, size_t numeric_support_depth)
    {
        const auto* annotation = this->m_workspace.numeric_annotations.find(head);
        if (annotation)
            extract_relaxed_plan(head, *annotation, state_context, numeric_support_depth);
    }

    void extract_relaxed_plan(FunctionHead head, const NumericAnnotation& annotation, const StateContext<Kind>& state_context, size_t numeric_support_depth)
    {
        if (mark(m_function_markings, head.get_index().relation, head.get_index().row))
            return;

        const auto* witness = std::get_if<datalog::WitnessAnnotation<Kind, ::tyr::formalism::FunctionTag>>(&annotation);
        if (witness)
            extract_relaxed_plan(*witness, state_context, numeric_support_depth);
    }

    void extract_numeric_constraint_support(::tyr::formalism::datalog::GroundBooleanOperatorView constraint,
                                            const StateContext<Kind>& state_context,
                                            size_t numeric_support_depth)
    {
        if (m_numeric_support_workspaces.size() <= numeric_support_depth)
            m_numeric_support_workspaces.emplace_back();

        const auto& selector = this->m_workspace.get_numeric_support_selector();
        selector.for_each_constraint_support(constraint,
                                             m_numeric_support_workspaces[numeric_support_depth].selection,
                                             datalog::SumAggregation {},
                                             [&](const auto head, const auto, const auto& annotation)
                                             { extract_relaxed_plan(head, annotation, state_context, numeric_support_depth + 1); });
    }

    template<::tyr::formalism::RelationKind R>
    void extract_relaxed_plan(const datalog::WitnessAnnotation<Kind, R>& witness, const StateContext<Kind>& state_context, size_t numeric_support_depth)
    {
        if (const auto action = this->get_action(witness))
        {
            const auto action_binding = this->get_action_binding(*action);
            m_relaxed_plan.insert(action_binding);
            if (this->is_action_applicable(m_executor, *action, state_context))
                m_preferred_actions.insert(action_binding);
        }

        this->for_each_witness_precondition(
            witness,
            [&](const auto precondition) { extract_relaxed_plan(precondition, state_context, numeric_support_depth); },
            [&](const auto constraint) { extract_numeric_constraint_support(constraint, state_context, numeric_support_depth); });
    }

    template<typename Relation, typename Row>
    static bool mark(std::vector<boost::dynamic_bitset<>>& markings, Relation relation, Row row)
    {
        const auto relation_index = ygg::uint_t(relation);
        const auto row_index = ygg::uint_t(row);
        assert(relation_index < markings.size());
        if (ygg::test(row_index, markings[relation_index]))
            return true;
        ygg::set(row_index, true, markings[relation_index]);
        return false;
    }

    std::vector<boost::dynamic_bitset<>> m_predicate_markings;
    std::vector<boost::dynamic_bitset<>> m_function_markings;
    ActionExecutor m_executor;
    std::deque<NumericSupportWorkspace> m_numeric_support_workspaces;
    ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView> m_relaxed_plan;
    ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView> m_preferred_actions;
};

template<TaskKind Kind>
FFRPGHeuristic<Kind>::FFRPGHeuristic(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    m_impl(std::make_unique<Impl>(std::move(task), std::move(execution_context), cost_mode))
{
}

template<TaskKind Kind>
FFRPGHeuristic<Kind>::FFRPGHeuristic(std::unique_ptr<Impl> impl) : m_impl(std::move(impl))
{
}

template<TaskKind Kind>
FFRPGHeuristic<Kind>::~FFRPGHeuristic() = default;

template<TaskKind Kind>
FFRPGHeuristic<Kind>::FFRPGHeuristic(FFRPGHeuristic&&) noexcept = default;

template<TaskKind Kind>
FFRPGHeuristic<Kind>& FFRPGHeuristic<Kind>::operator=(FFRPGHeuristic&&) noexcept = default;

template<TaskKind Kind>
FFRPGHeuristicPtr<Kind> FFRPGHeuristic<Kind>::create(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{
    return std::make_shared<FFRPGHeuristic<Kind>>(std::move(task), std::move(execution_context), cost_mode);
}

template<TaskKind Kind>
void FFRPGHeuristic<Kind>::set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal)
{
    m_impl->set_goal(goal);
}

template<TaskKind Kind>
ygg::float_t FFRPGHeuristic<Kind>::evaluate(const ygg::Builder<State<Kind>>& state)
{
    return m_impl->evaluate(state);
}

template<TaskKind Kind>
HeuristicPtr<Kind> FFRPGHeuristic<Kind>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    return HeuristicPtr<Kind>(new FFRPGHeuristic(std::make_unique<Impl>(*m_impl, std::move(execution_context))));
}

template<TaskKind Kind>
const ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView>& FFRPGHeuristic<Kind>::get_preferred_actions()
{
    return m_impl->get_preferred_actions();
}

template<TaskKind Kind>
void FFRPGHeuristic<Kind>::print_summary(size_t verbosity) const
{
    m_impl->print_summary(verbosity);
}

template class FFRPGHeuristic<GroundTag>;
template class FFRPGHeuristic<LiftedTag>;

}
