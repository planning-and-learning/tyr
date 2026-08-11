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

#ifndef TYR_SRC_PLANNING_HEURISTICS_RPG_HPP_
#define TYR_SRC_PLANNING_HEURISTICS_RPG_HPP_

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/ground/solver.hpp"
#include "tyr/datalog/lifted/solver.hpp"
#include "tyr/datalog/policies/annotation_concept.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/cost_concept.hpp"
#include "tyr/datalog/policies/termination_concept.hpp"
#include "tyr/datalog/workspaces/program.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/programs/rpg.hpp"
#include "tyr/planning/task_utils.hpp"

#include <concepts>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning::detail
{

template<TaskKind Kind>
struct RPGDefinition
{
    TaskPtr<Kind> task;
    RPGProgram<Kind> rpg_program;

    RPGDefinition(TaskPtr<Kind> task, CostMode cost_mode) : task(std::move(task)), rpg_program(this->task->get_task(), cost_mode) {}
};

template<TaskKind Kind>
struct RPGPolicy;

inline ::tyr::formalism::planning::ActionBindingView to_action_binding(::tyr::formalism::planning::GroundActionView action) noexcept
{
    return action.get_row();
}

inline ::tyr::formalism::planning::ActionBindingView to_action_binding(::tyr::formalism::planning::ActionBindingView action) noexcept { return action; }

template<TaskKind Kind, typename Workspace>
void materialize_goal(RPGDefinition<Kind>& definition, Workspace& workspace, ::tyr::formalism::planning::GroundConjunctiveConditionView source_goal)
{
    namespace fd = ::tyr::formalism::datalog;
    auto& destination = [&]() -> fd::Repository&
    {
        if constexpr (std::same_as<Kind, GroundTag>)
            return definition.rpg_program.get_datalog_program().get_program_repository();
        else
            return workspace.workspace_repository;
    }();
    auto merge_context = ::tyr::formalism::planning::MergeDatalogContext { workspace.datalog_builder, destination };
    auto condition = fd::checkout<fd::GroundConjunctiveCondition>(merge_context.builder);

    const auto translate_atom = [&](const auto atom)
    {
        const auto& p2d = definition.rpg_program.get_translation_context().p2d;
        if constexpr (std::same_as<Kind, GroundTag>)
            return p2d.fluent_to_fluent_atom.at(atom);
        else
            return ::tyr::formalism::planning::merge_p2d(atom, p2d.fluent_to_fluent_predicate, merge_context).first;
    };

    for (const auto fact : source_goal.template get_facts<::tyr::formalism::PositiveTag>())
    {
        const auto atom = fact.get_atom();
        if (!atom)
            continue;

        auto literal = fd::checkout<fd::GroundLiteral<::tyr::formalism::FluentTag>>(merge_context.builder);
        literal->atom = translate_atom(*atom).get_index();
        literal->polarity = true;
        condition->fluent_literals.push_back(fd::get_or_create(merge_context.destination, *literal).first.get_index());
    }

    for (const auto numeric_constraint : source_goal.get_numeric_constraints())
        condition->numeric_constraints.push_back(::tyr::formalism::planning::merge_p2d(numeric_constraint, merge_context));

    workspace.tp.set_goals(fd::get_or_create(merge_context.destination, *condition).first);
}

template<typename Derived,
         TaskKind Kind,
         datalog::AnnotationPolicyConcept AP,
         datalog::TerminationPolicyConcept TP,
         datalog::RuleCostPolicyConcept CP = datalog::RuleCostPolicy>
class RPGEvaluator
{
public:
    using Definition = RPGDefinition<Kind>;
    using Policy = RPGPolicy<Kind>;
    using Workspace = datalog::ProgramWorkspace<Kind, AP, TP, CP>;

    RPGEvaluator(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::GENERAL) :
        RPGEvaluator(std::make_shared<Definition>(std::move(task), cost_mode), std::move(execution_context))
    {
    }

    RPGEvaluator(std::shared_ptr<Definition> definition, ygg::ExecutionContextPtr execution_context) :
        m_definition(std::move(definition)),
        m_execution_context(std::move(execution_context)),
        m_source_goal(m_definition->task->get_task().get_goal()),
        m_workspace(m_definition->rpg_program.get_datalog_program(), AP {}, TP {}, make_cost_policy(*m_definition))
    {
        Policy::set_goal(*m_definition, m_workspace, m_source_goal);
    }

    RPGEvaluator(const RPGEvaluator& source, ygg::ExecutionContextPtr execution_context) :
        m_definition(source.m_definition),
        m_execution_context(std::move(execution_context)),
        m_source_goal(source.m_source_goal),
        m_workspace(m_definition->rpg_program.get_datalog_program(), AP {}, TP {}, make_cost_policy(*m_definition))
    {
        Policy::set_goal(*m_definition, m_workspace, m_source_goal);
    }

    void set_goal(::tyr::formalism::planning::GroundConjunctiveConditionView goal)
    {
        m_source_goal = goal;
        Policy::set_goal(*m_definition, m_workspace, m_source_goal);
    }

    ygg::float_t evaluate(const ygg::Builder<State<Kind>>& state)
    {
        begin_state_evaluation();
        return evaluate_current_state(state);
    }

    void print_summary(size_t verbosity) const { Policy::print_summary(m_workspace, verbosity); }

protected:
    void begin_state_evaluation() { Policy::begin_state_evaluation(*m_definition, m_workspace, m_source_goal); }

    ygg::float_t evaluate_current_state(const ygg::Builder<State<Kind>>& state)
    {
        const auto& repository = *m_definition->task->get_repository();
        const auto& translation_context = m_definition->rpg_program.get_translation_context().p2d;
        insert_unextended_state(state, repository, translation_context, m_workspace);
        auto ctx = datalog::ProgramExecutionContext(m_workspace);
        if constexpr (std::same_as<Kind, LiftedTag>)
            ctx.set_num_threads(m_execution_context->get_num_threads());
        m_execution_context->arena().execute([&] { datalog::compute_model(ctx); });

        return m_workspace.tp.check(datalog::FactSets { m_workspace.const_workspace.facts.fact_sets, m_workspace.facts.fact_sets }) ?
                   static_cast<Derived&>(*this).compute_result(state) :
                   std::numeric_limits<ygg::float_t>::infinity();
    }

    ygg::float_t compute_result(const ygg::Builder<State<Kind>>&) const noexcept { return get_goal_cost(); }

    void set_action_binding_cost(::tyr::formalism::planning::ActionBindingView action_binding, datalog::Cost cost)
        requires std::same_as<CP, datalog::RuleCostOverridePolicy<Kind>>
    {
        m_workspace.cost_policy.set_action_cost(action_binding, cost);
    }

    datalog::Cost get_predicate_cost(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head) const noexcept
    {
        const auto* annotation = m_workspace.annotations.find(head);
        return annotation ? datalog::get_cost(*annotation) : datalog::Cost(0);
    }

    datalog::Cost get_goal_cost() const noexcept
    {
        const auto& numeric_support_selector = m_workspace.get_numeric_support_selector();
        return m_workspace.tp.get_total_cost(datalog::FactSets { m_workspace.const_workspace.facts.fact_sets, m_workspace.facts.fact_sets },
                                             m_workspace.annotations,
                                             m_workspace.numeric_annotations,
                                             numeric_support_selector);
    }

    template<::tyr::formalism::RelationKind R>
    auto get_action(const datalog::WitnessAnnotation<R>& witness)
    {
        return Policy::get_action(*m_definition, m_workspace, witness);
    }

    template<::tyr::formalism::RelationKind R>
    std::optional<::tyr::formalism::planning::ActionBindingView> get_action_binding(const datalog::WitnessAnnotation<R>& witness)
    {
        const auto action = get_action(witness);
        if (!action)
            return std::nullopt;
        return detail::to_action_binding(*action);
    }

    ::tyr::formalism::planning::ActionBindingView get_action_binding(typename Policy::Action action) const noexcept
    {
        return detail::to_action_binding(action);
    }

    template<typename Executor>
    bool is_action_applicable(Executor& executor, typename Policy::Action action, const StateContext<Kind>& state_context)
    {
        return Policy::is_action_applicable(*m_definition, m_workspace, executor, action, state_context);
    }

    void append_planning_cut_frontier_atom(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head,
                                           ::tyr::formalism::planning::GroundAtomViewList<::tyr::formalism::FluentTag>& atoms)
    {
        if (const auto atom = Policy::translate_cut_atom(*m_definition, m_workspace, head))
            atoms.push_back(*atom);
    }

    template<::tyr::formalism::RelationKind R, typename Callback>
    void for_each_witness_precondition(const datalog::WitnessAnnotation<R>& witness, Callback&& callback)
    {
        Policy::visit_witness_rule_instance(m_workspace,
                                            witness,
                                            [&](const auto& instance)
                                            {
                                                for (const auto literal : instance.get_body().template get_literals<::tyr::formalism::FluentTag>())
                                                    if (literal.get_polarity())
                                                        callback(instance.resolve(literal.get_atom()));
                                            });
    }

    template<typename Callback>
    void for_each_achiever(::tyr::formalism::datalog::PredicateBindingView<::tyr::formalism::FluentTag> head, Callback&& callback)
        requires AP::records_propositional_achievers
    {
        const auto* achievers = m_workspace.annotation_policy.find_achievers(head);
        if (!achievers)
            return;

        for (const auto& achiever : *achievers)
            callback(achiever);
    }

    const Task<Kind>& get_task() const noexcept { return *m_definition->task; }

    std::shared_ptr<Definition> m_definition;
    ygg::ExecutionContextPtr m_execution_context;
    ::tyr::formalism::planning::GroundConjunctiveConditionView m_source_goal;
    Workspace m_workspace;

private:
    static CP make_cost_policy(const Definition& definition)
    {
        if constexpr (std::same_as<CP, datalog::RuleCostOverridePolicy<Kind>>)
            return CP(definition.rpg_program.template get_rule_to_action_mapping<::tyr::formalism::PredicateTag>(),
                      definition.rpg_program.template get_rule_to_action_mapping<::tyr::formalism::FunctionTag>());
        else
            return CP {};
    }
};

}

#endif
