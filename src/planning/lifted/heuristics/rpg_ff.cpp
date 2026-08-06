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

#include "tyr/planning/lifted/heuristics/rpg_ff.hpp"

#include "rpg.hpp"
#include "tyr/datalog/lifted/policies/annotation.hpp"
#include "tyr/datalog/policies/termination.hpp"
#include "tyr/formalism/datalog/expression_properties.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/planning/action_executor.hpp"
#include "tyr/planning/applicability.hpp"

#include <boost/dynamic_bitset.hpp>
#include <cassert>

namespace f = tyr::formalism;

namespace tyr::planning
{
struct FFRPGHeuristic<LiftedTag>::Impl :
    detail::LiftedRPGBase<Impl,
                          datalog::OrAnnotationPolicy<LiftedTag>,
                          datalog::AndAnnotationPolicy<LiftedTag, datalog::SumAggregation>,
                          datalog::TerminationPolicy<LiftedTag, datalog::SumAggregation>>
{
    using Base = detail::LiftedRPGBase<Impl,
                                       datalog::OrAnnotationPolicy<LiftedTag>,
                                       datalog::AndAnnotationPolicy<LiftedTag, datalog::SumAggregation>,
                                       datalog::TerminationPolicy<LiftedTag, datalog::SumAggregation>>;

    Impl(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode);
    Impl(const Impl& source, ygg::ExecutionContextPtr execution_context);

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal);
    ygg::float_t evaluate(const ygg::Builder<State<LiftedTag>>& state)
    {
        m_numeric_support_selector_workspace.clear();
        m_relaxed_plan.clear();
        m_preferred_actions.clear();
        return Base::evaluate(state);
    }
    ygg::float_t compute_result(const ygg::Builder<State<LiftedTag>>& state);
    const ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView>& get_preferred_actions() const noexcept;

private:
    bool mark_atom(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> atom);
    bool mark_function(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> function);
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

    std::vector<boost::dynamic_bitset<>> m_markings;
    std::vector<boost::dynamic_bitset<>> m_function_markings;
    ygg::IndexList<::tyr::formalism::Object> m_binding;
    ActionExecutor m_executor;
    datalog::NumericSupportSelectorWorkspace<LiftedTag> m_numeric_support_selector_workspace;
    ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView> m_relaxed_plan;
    ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView> m_preferred_actions;
};

FFRPGHeuristic<LiftedTag>::Impl::Impl(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    Base(std::move(task),
         std::move(execution_context),
         datalog::OrAnnotationPolicy<LiftedTag> {},
         datalog::AndAnnotationPolicy<LiftedTag, datalog::SumAggregation> {},
         cost_mode),
    m_markings(m_definition->rpg_program.get_datalog_program().get_program().get_predicates<::tyr::formalism::FluentTag>().size()),
    m_function_markings(m_definition->rpg_program.get_datalog_program().get_program().get_functions<::tyr::formalism::FluentTag>().size()),
    m_binding(),
    m_executor(),
    m_numeric_support_selector_workspace(),
    m_relaxed_plan(),
    m_preferred_actions()
{
}

FFRPGHeuristic<LiftedTag>::Impl::Impl(const Impl& source, ygg::ExecutionContextPtr execution_context) :
    Base(source.m_definition,
         source.m_task,
         std::move(execution_context),
         datalog::OrAnnotationPolicy<LiftedTag> {},
         datalog::AndAnnotationPolicy<LiftedTag, datalog::SumAggregation> {},
         source.m_source_goal),
    m_markings(m_definition->rpg_program.get_datalog_program().get_program().get_predicates<::tyr::formalism::FluentTag>().size()),
    m_function_markings(m_definition->rpg_program.get_datalog_program().get_program().get_functions<::tyr::formalism::FluentTag>().size())
{
}

void FFRPGHeuristic<LiftedTag>::Impl::set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal)
{
    Base::set_goal(goal);
    m_relaxed_plan.clear();
    m_preferred_actions.clear();
}

ygg::float_t FFRPGHeuristic<LiftedTag>::Impl::compute_result(const ygg::Builder<State<LiftedTag>>& state)
{
    for (auto& bitset : m_markings)
        bitset.reset();
    for (auto& bitset : m_function_markings)
        bitset.reset();

    auto state_context = StateContext<LiftedTag>(*this->m_task, state, ygg::float_t(0));
    auto grounder_context = ::tyr::formalism::planning::GrounderContext { this->m_workspace.planning_builder, *this->m_task->get_repository(), m_binding };

    if (const auto& goal = m_workspace.tp.get_goal())
    {
        for (const auto literal : goal->get_literals<::tyr::formalism::FluentTag>())
        {
            assert(literal.get_polarity());

            extract_relaxed_plan_and_preferred_actions(literal.get_atom().get_row(), state_context, grounder_context);
        }

        for (const auto constraint : goal->get_numeric_constraints())
            extract_numeric_constraint_support(constraint, state_context, grounder_context);
    }

    return m_relaxed_plan.size();
}

const ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView>& FFRPGHeuristic<LiftedTag>::Impl::get_preferred_actions() const noexcept
{
    return m_preferred_actions;
}

bool FFRPGHeuristic<LiftedTag>::Impl::mark_atom(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> binding)
{
    const auto g = ygg::uint_t(binding.get_index().relation);
    const auto i = ygg::uint_t(binding.get_index().row);

    assert(g < m_markings.size());
    if (ygg::test(i, m_markings[g]))
        return true;
    ygg::set(i, true, m_markings[g]);
    return false;
}

bool FFRPGHeuristic<LiftedTag>::Impl::mark_function(::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding)
{
    const auto g = ygg::uint_t(binding.get_index().relation);
    const auto i = ygg::uint_t(binding.get_index().row);

    assert(g < m_function_markings.size());
    if (ygg::test(i, m_function_markings[g]))
        return true;
    ygg::set(i, true, m_function_markings[g]);
    return false;
}

void FFRPGHeuristic<LiftedTag>::Impl::extract_relaxed_plan_and_preferred_actions(
    ::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> binding,
    const StateContext<LiftedTag>& state_context,
    ::tyr::formalism::planning::GrounderContext& grounder_context)
{
    // Base case 1: atom is already marked => do not recurse again
    if (mark_atom(binding))
        return;

    // Base case 2: atom is initially true, i.e., has no witness => do not recurse again
    const auto* annotation = m_workspace.and_annot.find(binding);
    if (!annotation)
        return;

    const auto* witness = std::get_if<datalog::WitnessAnnotation<LiftedTag>>(annotation);
    if (!witness)
        return;

    extract_relaxed_plan_and_preferred_actions(*witness, state_context, grounder_context);
}

void FFRPGHeuristic<LiftedTag>::Impl::extract_relaxed_plan_and_preferred_actions(
    ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding,
    const StateContext<LiftedTag>& state_context,
    ::tyr::formalism::planning::GrounderContext& grounder_context)
{
    const auto* annotation = m_workspace.numeric_and_annot.find(binding);
    if (!annotation)
        return;

    extract_relaxed_plan_and_preferred_actions(binding, *annotation, state_context, grounder_context);
}

void FFRPGHeuristic<LiftedTag>::Impl::extract_relaxed_plan_and_preferred_actions(
    ::tyr::formalism::datalog::FunctionBindingView<::tyr::formalism::FluentTag> binding,
    const datalog::Annotation<LiftedTag, f::FunctionTag>& annotation,
    const StateContext<LiftedTag>& state_context,
    ::tyr::formalism::planning::GrounderContext& grounder_context)
{
    // Base case 1: function binding is already marked => do not recurse again
    if (mark_function(binding))
        return;

    // Base case 2: function binding is initially assigned, i.e., has no witness => do not recurse again
    const auto* witness = std::get_if<datalog::WitnessAnnotation<LiftedTag, f::FunctionTag>>(&annotation);
    if (!witness)
        return;

    extract_relaxed_plan_and_preferred_actions(*witness, state_context, grounder_context);
}

void FFRPGHeuristic<LiftedTag>::Impl::extract_numeric_constraint_support(::tyr::formalism::datalog::GroundBooleanOperatorView constraint,
                                                                         const StateContext<LiftedTag>& state_context,
                                                                         ::tyr::formalism::planning::GrounderContext& grounder_context)
{
    m_workspace.numeric_support_selector->for_each_constraint_support(
        constraint,
        m_numeric_support_selector_workspace,
        datalog::SumAggregation {},
        [&](const auto binding, const auto, const auto& annotation)
        { extract_relaxed_plan_and_preferred_actions(binding, annotation, state_context, grounder_context); });
}

template<f::RelationKind R>
void FFRPGHeuristic<LiftedTag>::Impl::extract_relaxed_plan_and_preferred_actions(const datalog::WitnessAnnotation<LiftedTag, R>& witness,
                                                                                 const StateContext<LiftedTag>& state_context,
                                                                                 ::tyr::formalism::planning::GrounderContext& grounder_context)
{
    const auto rule_row = witness.get_rule_key();
    const auto rule = rule_row.get_relation();
    const auto row = rule_row.get_objects();

    if (const auto action_binding = this->get_action_binding(witness))
    {
        grounder_context.binding.clear();
        for (const auto object : action_binding->get_data())
            grounder_context.binding.push_back(object);

        m_relaxed_plan.insert(*action_binding);

        if (m_executor.is_applicable(action_binding->get_relation(), state_context, grounder_context, *m_task->get_fdr_context()))
            m_preferred_actions.insert(*action_binding);
    }

    // Divide case: recursively call for preconditions.

    auto datalog_grounder_context =
        ::tyr::formalism::datalog::GrounderContext { m_workspace.datalog_builder, m_workspace.workspace_repository, m_workspace.binding };
    const auto& const_rule_workspace = *m_definition->rpg_program.get_const_program_workspace().template get_rules<R>()[ygg::uint_t(rule.get_index())];

    const auto witness_condition = const_rule_workspace.get_witness_rule().get_body();

    for (const auto literal : witness_condition.template get_literals<::tyr::formalism::FluentTag>())
    {
        // Cannot do this before the loop because of overwrites during recursion; we could binding from a builder and place it into the grounder context.
        datalog_grounder_context.binding.clear();
        for (const auto object : row)
            datalog_grounder_context.binding.push_back(object.get_index());

        const auto witness_atom = ::tyr::formalism::datalog::ground(literal.get_atom(), datalog_grounder_context).first;

        extract_relaxed_plan_and_preferred_actions(witness_atom.get_row(), state_context, grounder_context);
    }

    for (const auto constraint : witness_condition.get_numeric_constraints())
    {
        // Cannot do this before the loop because of overwrites during recursion; we could binding from a builder and place it into the grounder context.
        datalog_grounder_context.binding.clear();
        for (const auto object : row)
            datalog_grounder_context.binding.push_back(object.get_index());

        const auto ground_constraint = ::tyr::formalism::datalog::ground(constraint, datalog_grounder_context);
        extract_numeric_constraint_support(ground_constraint, state_context, grounder_context);
    }
}

FFRPGHeuristic<LiftedTag>::FFRPGHeuristic(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode) :
    m_impl(std::make_unique<Impl>(std::move(task), std::move(execution_context), cost_mode))
{
}

FFRPGHeuristic<LiftedTag>::FFRPGHeuristic(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

FFRPGHeuristic<LiftedTag>::~FFRPGHeuristic() = default;
FFRPGHeuristic<LiftedTag>::FFRPGHeuristic(FFRPGHeuristic&&) noexcept = default;
FFRPGHeuristic<LiftedTag>& FFRPGHeuristic<LiftedTag>::operator=(FFRPGHeuristic&&) noexcept = default;

FFRPGHeuristicPtr<LiftedTag> FFRPGHeuristic<LiftedTag>::create(TaskPtr<LiftedTag> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode)
{
    return std::make_shared<FFRPGHeuristic<LiftedTag>>(std::move(task), std::move(execution_context), cost_mode);
}

void FFRPGHeuristic<LiftedTag>::set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal) { m_impl->set_goal(goal); }
ygg::float_t FFRPGHeuristic<LiftedTag>::evaluate(const ygg::Builder<State<LiftedTag>>& state) { return m_impl->evaluate(state); }
HeuristicPtr<LiftedTag> FFRPGHeuristic<LiftedTag>::make_worker(ygg::ExecutionContextPtr execution_context) const
{
    return HeuristicPtr<LiftedTag>(new FFRPGHeuristic(std::make_unique<Impl>(*m_impl, std::move(execution_context))));
}
const ygg::UnorderedSet<::tyr::formalism::planning::ActionBindingView>& FFRPGHeuristic<LiftedTag>::get_preferred_actions()
{
    return m_impl->get_preferred_actions();
}
void FFRPGHeuristic<LiftedTag>::print_summary(size_t verbosity) const { m_impl->print_summary(verbosity); }

}
