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

#include "tyr/planning/ground/heuristics/rpg_ff.hpp"

#include "rpg.hpp"
#include "tyr/datalog/ground/policies/annotation.hpp"
#include "tyr/datalog/ground/policies/cost.hpp"
#include "tyr/datalog/ground/policies/numeric_support.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/planning/applicability.hpp"

#include <boost/dynamic_bitset.hpp>
#include <utility>
#include <vector>

namespace f = tyr::formalism;
namespace fd = tyr::formalism::datalog;

namespace tyr::planning
{

struct FFRPGHeuristic<GroundTag>::Impl :
    detail::RPGEvaluator<Impl,
                         GroundTag,
                         datalog::OrAnnotationPolicy<GroundTag>,
                         datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                         datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>>
{
    using Base = detail::RPGEvaluator<Impl,
                                      GroundTag,
                                      datalog::OrAnnotationPolicy<GroundTag>,
                                      datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>,
                                      datalog::TerminationPolicy<GroundTag, datalog::SumAggregation>>;

    Impl(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
        Base(std::move(task),
             std::move(execution_context),
             datalog::OrAnnotationPolicy<GroundTag>(),
             datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>(),
             cost_mode),
        m_markings(1),
        m_function_markings(),
        m_numeric_support_selector_workspace(),
        m_effect_families(),
        m_relaxed_plan(),
        m_preferred_actions()
    {
        m_markings.front().resize(get_program().get_datalog_program().get_program().get_atoms<f::FluentTag>().size());
    }

    Impl(const Impl& source, ygg::ExecutionContextPtr execution_context) :
        Base(source,
             std::move(execution_context),
             datalog::OrAnnotationPolicy<GroundTag>(),
             datalog::AndAnnotationPolicy<GroundTag, datalog::SumAggregation>()),
        m_markings(1),
        m_function_markings(),
        m_numeric_support_selector_workspace(),
        m_effect_families(),
        m_relaxed_plan(),
        m_preferred_actions()
    {
        m_markings.front().resize(get_program().get_datalog_program().get_program().get_atoms<f::FluentTag>().size());
    }

    void set_goal(f::planning::GroundConjunctiveConditionView goal)
    {
        Base::set_goal(goal);
        m_relaxed_plan.clear();
        m_preferred_actions.clear();
    }

    ygg::float_t evaluate(const ygg::Builder<State<GroundTag>>& state)
    {
        m_relaxed_plan.clear();
        m_preferred_actions.clear();
        return Base::evaluate(state);
    }

    ygg::float_t compute_result(const ygg::Builder<State<GroundTag>>& state);
    bool mark_atom(fd::GroundAtomView<f::FluentTag> atom);
    bool mark_function(fd::GroundFunctionTermView<f::FluentTag> term);
    void extract_relaxed_plan_and_preferred_actions(fd::GroundAtomView<f::FluentTag> atom, const StateContext<GroundTag>& state_context);
    void extract_relaxed_plan_and_preferred_actions(fd::GroundFunctionTermView<f::FluentTag> term, const StateContext<GroundTag>& state_context);
    void extract_relaxed_plan_and_preferred_actions(fd::GroundFunctionTermView<f::FluentTag> term,
                                                    const datalog::Annotation<GroundTag, f::FunctionTag>& annotation,
                                                    const StateContext<GroundTag>& state_context);
    void extract_numeric_constraint_support(fd::GroundBooleanOperatorView constraint, const StateContext<GroundTag>& state_context);
    template<f::RelationKind R>
    void extract_relaxed_plan_and_preferred_actions(const datalog::WitnessAnnotation<GroundTag, R>& witness, const StateContext<GroundTag>& state_context);

    std::vector<boost::dynamic_bitset<>> m_markings;
    ygg::UnorderedSet<fd::GroundFunctionTermView<f::FluentTag>> m_function_markings;
    datalog::GroundNumericSupportSelectorWorkspace m_numeric_support_selector_workspace;
    f::planning::EffectFamilyList m_effect_families;
    ygg::UnorderedSet<ygg::Index<f::planning::GroundAction>> m_relaxed_plan;
    ygg::UnorderedSet<f::planning::ActionBindingView> m_preferred_actions;
};

FFRPGHeuristic<GroundTag>::FFRPGHeuristic(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    m_impl(std::make_unique<Impl>(std::move(task), std::move(execution_context), cost_mode))
{
}

FFRPGHeuristic<GroundTag>::FFRPGHeuristic(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

FFRPGHeuristic<GroundTag>::~FFRPGHeuristic() = default;
FFRPGHeuristic<GroundTag>::FFRPGHeuristic(FFRPGHeuristic&&) noexcept = default;
FFRPGHeuristic<GroundTag>& FFRPGHeuristic<GroundTag>::operator=(FFRPGHeuristic&&) noexcept = default;

FFRPGHeuristicPtr<GroundTag> FFRPGHeuristic<GroundTag>::create(TaskPtr<GroundTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{
    return std::make_shared<FFRPGHeuristic<GroundTag>>(std::move(task), std::move(execution_context), cost_mode);
}

void FFRPGHeuristic<GroundTag>::set_goal(f::planning::GroundConjunctiveConditionView goal) { m_impl->set_goal(goal); }

ygg::float_t FFRPGHeuristic<GroundTag>::evaluate(const ygg::Builder<State<GroundTag>>& state) { return m_impl->evaluate(state); }

HeuristicPtr<GroundTag> FFRPGHeuristic<GroundTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    return HeuristicPtr<GroundTag>(new FFRPGHeuristic(std::make_unique<Impl>(*m_impl, std::move(execution_context))));
}

const ygg::UnorderedSet<f::planning::ActionBindingView>& FFRPGHeuristic<GroundTag>::get_preferred_actions() { return m_impl->m_preferred_actions; }

ygg::float_t FFRPGHeuristic<GroundTag>::Impl::compute_result(const ygg::Builder<State<GroundTag>>& state)
{
    m_function_markings.clear();
    m_numeric_support_selector_workspace.clear();
    for (auto& bitset : m_markings)
        bitset.reset();

    const auto state_context = StateContext<GroundTag>(get_task(), state, ygg::float_t(0));
    if (const auto& goal = m_workspace.tp.get_goal())
    {
        for (const auto literal : goal->get_literals<f::FluentTag>())
            if (literal.get_polarity())
                extract_relaxed_plan_and_preferred_actions(literal.get_atom(), state_context);

        for (const auto constraint : goal->get_numeric_constraints())
            extract_numeric_constraint_support(constraint, state_context);
    }

    return ygg::float_t(m_relaxed_plan.size());
}

bool FFRPGHeuristic<GroundTag>::Impl::mark_atom(fd::GroundAtomView<f::FluentTag> atom)
{
    const auto i = ygg::uint_t(atom.get_index());
    if (m_markings.front().size() <= i)
        m_markings.front().resize(i + 1);
    if (ygg::test(i, m_markings.front()))
        return true;
    ygg::set(i, true, m_markings.front());
    return false;
}

bool FFRPGHeuristic<GroundTag>::Impl::mark_function(fd::GroundFunctionTermView<f::FluentTag> term) { return !m_function_markings.insert(term).second; }

void FFRPGHeuristic<GroundTag>::Impl::extract_relaxed_plan_and_preferred_actions(fd::GroundAtomView<f::FluentTag> atom,
                                                                                 const StateContext<GroundTag>& state_context)
{
    if (mark_atom(atom))
        return;

    const auto* annotation = this->m_workspace.and_annot.find(atom.get_row());
    if (!annotation)
        return;

    const auto* witness = std::get_if<datalog::WitnessAnnotation<GroundTag>>(annotation);
    if (!witness)
        return;

    extract_relaxed_plan_and_preferred_actions(*witness, state_context);
}

void FFRPGHeuristic<GroundTag>::Impl::extract_relaxed_plan_and_preferred_actions(fd::GroundFunctionTermView<f::FluentTag> term,
                                                                                 const StateContext<GroundTag>& state_context)
{
    const auto* annotation = this->m_workspace.numeric_and_annot.find(term);
    if (!annotation)
        return;

    extract_relaxed_plan_and_preferred_actions(term, *annotation, state_context);
}

void FFRPGHeuristic<GroundTag>::Impl::extract_relaxed_plan_and_preferred_actions(fd::GroundFunctionTermView<f::FluentTag> term,
                                                                                 const datalog::Annotation<GroundTag, f::FunctionTag>& annotation,
                                                                                 const StateContext<GroundTag>& state_context)
{
    if (mark_function(term))
        return;

    const auto* witness = std::get_if<datalog::WitnessAnnotation<GroundTag, f::FunctionTag>>(&annotation);
    if (!witness)
        return;

    extract_relaxed_plan_and_preferred_actions(*witness, state_context);
}

void FFRPGHeuristic<GroundTag>::Impl::extract_numeric_constraint_support(fd::GroundBooleanOperatorView constraint, const StateContext<GroundTag>& state_context)
{
    const auto numeric_support_selector =
        datalog::GroundNumericSupportSelector(this->m_workspace.const_workspace.facts, this->m_workspace.facts, this->m_workspace.numeric_and_annot);
    numeric_support_selector.for_each_constraint_support(constraint,
                                                         m_numeric_support_selector_workspace,
                                                         datalog::SumAggregation {},
                                                         [&](const auto term, const auto, const auto& annotation)
                                                         { extract_relaxed_plan_and_preferred_actions(term, annotation, state_context); });
}

template<f::RelationKind R>
void FFRPGHeuristic<GroundTag>::Impl::extract_relaxed_plan_and_preferred_actions(const datalog::WitnessAnnotation<GroundTag, R>& witness,
                                                                                 const StateContext<GroundTag>& state_context)
{
    const auto rule = witness.get_rule_key();
    const auto& mapping = get_program().template get_rule_to_action_mapping<R>();
    if (const auto it = mapping.find(rule); it != mapping.end())
    {
        const auto action = it->second;
        m_relaxed_plan.insert(action.get_index());
        m_effect_families.clear();
        if (is_applicable(action, state_context, m_effect_families))
            m_preferred_actions.insert(action.get_row());
    }

    for (const auto literal : rule.get_body().template get_literals<f::FluentTag>())
    {
        if (literal.get_polarity())
            extract_relaxed_plan_and_preferred_actions(literal.get_atom(), state_context);
    }

    for (const auto constraint : rule.get_body().get_numeric_constraints())
        extract_numeric_constraint_support(constraint, state_context);
}

}
