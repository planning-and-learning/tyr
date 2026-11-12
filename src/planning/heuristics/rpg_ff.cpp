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
#include "tyr/datalog/policies/termination.hpp"

#include <cassert>
#include <tuple>
#include <urd/elements/flat.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/repr/values/dense_set.hpp>
#include <urd/repr/values/std_set.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>
#include <yggdrasil/core/config.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
struct FFRPGHeuristic<Kind>::Impl :
    detail::RPGEvaluator<Impl, Kind, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>, datalog::TerminationPolicy<datalog::SumAggregation>>
{
    using Base =
        detail::RPGEvaluator<Impl, Kind, datalog::MinCostAnnotationPolicy<datalog::SumAggregation>, datalog::TerminationPolicy<datalog::SumAggregation>>;
    using PredicateHead = formalism::datalog::PredicateRowView<formalism::FluentTag>;
    Impl(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
        Base(std::move(task), std::move(execution_context), cost_mode),
        m_predicate_markings(this->workspace().facts.fact_sets.predicate.size())
    {
    }

    Impl(const Impl& source, ygg::ExecutionContextPtr execution_context) :
        Base(source, std::move(execution_context)),
        m_predicate_markings(this->workspace().facts.fact_sets.predicate.size())
    {
    }

    void set_goal(formalism::planning::ConjunctiveConditionView<GroundTag> goal)
    {
        Base::set_goal(goal);
        m_relaxed_plan.clear();
    }

    ygg::float_t evaluate(const urd::data<State<Kind>>& state)
    {
        m_relaxed_plan.clear();
        return Base::evaluate(state);
    }

    ygg::float_t compute_result(const urd::data<State<Kind>>&)
    {
        for (auto& markings : m_predicate_markings)
            markings.clear();

        if (const auto& goal = this->workspace().tp.get_goal())
        {
            for (const auto literal : goal->template get_literals<formalism::FluentTag>())
            {
                assert(literal.get_polarity());
                extract_relaxed_plan(literal.get_atom().get_row());
            }
        }

        return static_cast<ygg::float_t>(m_relaxed_plan.size());
    }

    [[nodiscard]] const auto& get_relaxed_plan() const noexcept { return m_relaxed_plan; }

private:
    void extract_relaxed_plan(PredicateHead head)
    {
        assert(static_cast<ygg::uint_t>(head.data().relation.value) < m_predicate_markings.size());
        if (!m_predicate_markings[static_cast<ygg::uint_t>(head.data().relation.value)].insert(head.data().row).second)
            return;

        const auto* annotation = this->workspace().annotations.find(head.data());
        if (!annotation)
            return;

        const auto* witness = std::get_if<datalog::WitnessAnnotation>(annotation);
        if (witness)
            extract_relaxed_plan(*witness);
    }

    void extract_relaxed_plan(const datalog::WitnessAnnotation& witness)
    {
        if (const auto action_binding = this->get_action_binding(witness))
            m_relaxed_plan.insert(*action_binding);

        this->for_each_witness_precondition(witness, [&](const auto precondition) { extract_relaxed_plan(precondition); });
    }

    urd::Store<urd::sequence<urd::Store<urd::set<urd::index<ygg::Row<formalism::Predicate<formalism::FluentTag>>>>, urd::dense, urd::proportional_clear>>>
        m_predicate_markings;
    urd::Store<urd::set<formalism::planning::ActionRowView>> m_relaxed_plan;
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
{ return std::make_shared<FFRPGHeuristic<Kind>>(std::move(task), std::move(execution_context), cost_mode); }

template<TaskKind Kind>
void FFRPGHeuristic<Kind>::set_goal(formalism::planning::ConjunctiveConditionView<GroundTag> goal)
{ m_impl->set_goal(goal); }

template<TaskKind Kind>
ygg::float_t FFRPGHeuristic<Kind>::evaluate(const urd::data<State<Kind>>& state)
{ return m_impl->evaluate(state); }

template<TaskKind Kind>
HeuristicPtr<Kind> FFRPGHeuristic<Kind>::make_worker(ygg::ExecutionContextPtr execution_context) const
{ return HeuristicPtr<Kind>(new FFRPGHeuristic(std::make_unique<Impl>(*m_impl, std::move(execution_context)))); }

template<TaskKind Kind>
const urd::Store<urd::set<formalism::planning::ActionRowView>>* FFRPGHeuristic<Kind>::get_relaxed_plan() const
{ return &m_impl->get_relaxed_plan(); }

template<TaskKind Kind>
void FFRPGHeuristic<Kind>::print_summary(size_t verbosity) const
{ m_impl->print_summary(verbosity); }

template class FFRPGHeuristic<GroundTag>;
template class FFRPGHeuristic<LiftedTag>;

}
