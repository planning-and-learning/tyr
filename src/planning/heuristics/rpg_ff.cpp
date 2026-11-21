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

#include <boost/mp11.hpp>
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

    Impl(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) : Base(std::move(task), std::move(execution_context), cost_mode)
    { initialize_markings(); }

    Impl(const Impl& source, ygg::ExecutionContextPtr execution_context) : Base(source, std::move(execution_context)) { initialize_markings(); }

    void set_goal(formalism::ConjunctiveConditionView<GroundTag> goal)
    {
        Base::set_goal(goal);
        m_relaxed_plan.clear();
    }

    ygg::float_t evaluate(const StateView<Kind>& state)
    {
        m_relaxed_plan.clear();
        return Base::evaluate(state);
    }

    ygg::float_t compute_result(const StateView<Kind>&)
    {
        boost::mp11::mp_for_each<datalog::MutableTags>(
            [&]<typename F>(F)
            {
                for (auto& markings : m_predicate_markings.template get<F>())
                    markings.clear();
            });

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
    /// One marking per predicate of each mutable kind: the fluent atoms and the derived ones (projections of split rules).
    void initialize_markings()
    {
        boost::mp11::mp_for_each<datalog::MutableTags>(
            [&]<typename F>(F) { m_predicate_markings.template get<F>().resize(this->workspace().facts.fact_sets.template get<F>().predicate.size()); });
    }

    template<formalism::FactKind F>
    void extract_relaxed_plan(formalism::PredicateRowView<F> head)
    {
        auto& markings = m_predicate_markings.template get<F>();
        assert(static_cast<ygg::uint_t>(head.index().relation.value) < markings.size());
        if (!markings[static_cast<ygg::uint_t>(head.index().relation.value)].insert(head.index().row).second)
            return;

        const auto* annotation = this->workspace().annotations.find(head.index());
        if (!annotation)
            return;

        const auto* witness = std::get_if<datalog::WitnessAnnotation<Kind>>(annotation);
        if (witness)
            extract_relaxed_plan(*witness);
    }

    void extract_relaxed_plan(const datalog::WitnessAnnotation<Kind>& witness)
    {
        if (const auto action_binding = this->get_action_binding(witness))
            m_relaxed_plan.insert(*action_binding);

        this->for_each_witness_precondition(witness, [&](const auto precondition) { extract_relaxed_plan(precondition); });
    }

    /// The marked atoms of each predicate of kind F.
    template<typename F>
    using PredicateMarkings =
        urd::Store<urd::sequence<urd::Store<urd::set<urd::index<ygg::Row<formalism::Predicate<F>>>>, urd::dense, urd::proportional_clear>>>;

    datalog::MutableFamily<PredicateMarkings> m_predicate_markings;
    urd::Store<urd::set<formalism::ActionRowView>> m_relaxed_plan;
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
void FFRPGHeuristic<Kind>::set_goal(formalism::ConjunctiveConditionView<GroundTag> goal)
{ m_impl->set_goal(goal); }

template<TaskKind Kind>
ygg::float_t FFRPGHeuristic<Kind>::evaluate(const StateView<Kind>& state)
{ return m_impl->evaluate(state); }

template<TaskKind Kind>
HeuristicPtr<Kind> FFRPGHeuristic<Kind>::make_worker(ygg::ExecutionContextPtr execution_context) const
{ return HeuristicPtr<Kind>(new FFRPGHeuristic(std::make_unique<Impl>(*m_impl, std::move(execution_context)))); }

template<TaskKind Kind>
const urd::Store<urd::set<formalism::ActionRowView>>* FFRPGHeuristic<Kind>::get_relaxed_plan() const
{ return &m_impl->get_relaxed_plan(); }

template<TaskKind Kind>
void FFRPGHeuristic<Kind>::print_summary(size_t verbosity) const
{ m_impl->print_summary(verbosity); }

template class FFRPGHeuristic<GroundTag>;
template class FFRPGHeuristic<LiftedTag>;

}
