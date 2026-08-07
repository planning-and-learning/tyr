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

#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning::detail
{

template<::tyr::formalism::RelationKind R>
auto get_rpg_rules(::tyr::formalism::datalog::ProgramView<GroundTag> program) noexcept
{
    return program.template get_ground_rules<R>();
}

template<::tyr::formalism::RelationKind R>
auto get_rpg_rules(::tyr::formalism::datalog::ProgramView<LiftedTag> program) noexcept
{
    return program.template get_rules<R>();
}

template<TaskKind Kind>
bool needs_expanded_lmcut(::tyr::formalism::datalog::ProgramView<Kind> program)
{
    for (const auto rule : get_rpg_rules<::tyr::formalism::PredicateTag>(program))
        if (!rule.get_body().get_numeric_constraints().empty())
            return true;
    return !get_rpg_rules<::tyr::formalism::FunctionTag>(program).empty();
}

template<TaskKind Kind>
struct RPGDefinition
{
    TaskPtr<Kind> task;
    RPGProgram<Kind> rpg_program;
    CostMode cost_mode;
    bool use_expanded_lmcut;

    RPGDefinition(TaskPtr<Kind> task, CostMode cost_mode, bool compute_expanded_lmcut) :
        task(std::move(task)),
        rpg_program(this->task->get_task(), cost_mode),
        cost_mode(cost_mode),
        use_expanded_lmcut(compute_expanded_lmcut && needs_expanded_lmcut(rpg_program.get_datalog_program().get_program()))
    {
    }
};

template<TaskKind Kind>
struct RPGPolicy;

template<typename Workspace, typename TranslateAtom>
void materialize_goal(Workspace& workspace,
                      ::tyr::formalism::planning::GroundConjunctiveConditionView source_goal,
                      ::tyr::formalism::planning::MergeDatalogContext& merge_context,
                      TranslateAtom&& translate_atom)
{
    namespace fd = ::tyr::formalism::datalog;
    auto condition_ptr = merge_context.builder.template get_builder<fd::GroundConjunctiveCondition>();
    auto& condition = *condition_ptr;
    condition.clear();

    for (const auto fact : source_goal.template get_facts<::tyr::formalism::PositiveTag>())
    {
        const auto atom = fact.get_atom();
        if (!atom)
            continue;

        auto literal_ptr = merge_context.builder.template get_builder<fd::GroundLiteral<::tyr::formalism::FluentTag>>();
        auto& literal = *literal_ptr;
        literal.clear();
        literal.atom = translate_atom(*atom).get_index();
        literal.polarity = true;
        fd::canonicalize(literal);
        condition.fluent_literals.push_back(merge_context.destination.get_or_create(literal).first.get_index());
    }

    for (const auto numeric_constraint : source_goal.get_numeric_constraints())
        condition.numeric_constraints.push_back(::tyr::formalism::planning::merge_p2d(numeric_constraint, merge_context));

    fd::canonicalize(condition);
    workspace.tp.set_goals(merge_context.destination.get_or_create(condition).first);
}

template<typename Derived,
         TaskKind Kind,
         datalog::OrAnnotationPolicyConcept<Kind> OrAP,
         datalog::AndAnnotationPolicyConcept<Kind> AndAP,
         datalog::TerminationPolicyConcept<Kind> TP,
         datalog::RuleCostPolicyConcept<Kind> CP = datalog::RuleCostPolicy<Kind>>
class RPGEvaluator
{
public:
    using Definition = RPGDefinition<Kind>;
    using Policy = RPGPolicy<Kind>;
    using Workspace = datalog::ProgramWorkspace<Kind, OrAP, AndAP, TP, CP>;

    RPGEvaluator(TaskPtr<Kind> task,
                 ygg::ExecutionContextPtr execution_context,
                 OrAP or_ap,
                 AndAP and_ap,
                 CostMode cost_mode = CostMode::GENERAL,
                 bool compute_expanded_lmcut = false) :
        m_definition(std::make_shared<Definition>(std::move(task), cost_mode, compute_expanded_lmcut)),
        m_execution_context(std::move(execution_context)),
        m_source_goal(m_definition->task->get_task().get_goal()),
        m_workspace(m_definition->rpg_program.get_datalog_program(), std::move(or_ap), std::move(and_ap), TP {}, make_cost_policy(*m_definition))
    {
        Policy::set_goal(*m_definition, m_workspace, m_source_goal);
    }

    RPGEvaluator(const RPGEvaluator& source, ygg::ExecutionContextPtr execution_context, OrAP or_ap, AndAP and_ap) :
        m_definition(source.m_definition),
        m_execution_context(std::move(execution_context)),
        m_source_goal(source.m_source_goal),
        m_workspace(m_definition->rpg_program.get_datalog_program(), std::move(or_ap), std::move(and_ap), TP {}, make_cost_policy(*m_definition))
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
    constexpr const auto& self() const { return static_cast<const Derived&>(*this); }
    constexpr auto& self() { return static_cast<Derived&>(*this); }

    void begin_state_evaluation() { Policy::begin_state_evaluation(*m_definition, m_workspace, m_source_goal); }

    ygg::float_t evaluate_current_state(const ygg::Builder<State<Kind>>& state)
    {
        m_workspace.facts.reset();
        Policy::insert_state_facts(*m_definition, m_workspace, state);
        auto ctx = datalog::ProgramExecutionContext(m_workspace);
        m_execution_context->arena().execute([&] { datalog::compute_model(ctx); });

        return m_workspace.tp.check(datalog::FactSets { m_workspace.const_workspace.facts.fact_sets, m_workspace.facts.fact_sets }) ?
                   self().compute_result(state) :
                   std::numeric_limits<ygg::float_t>::infinity();
    }

    ygg::float_t compute_result(const ygg::Builder<State<Kind>>&) const noexcept { return get_goal_cost(); }

    void set_action_binding_cost(::tyr::formalism::planning::ActionBindingView action_binding, datalog::Cost cost)
        requires std::same_as<CP, datalog::RuleCostOverridePolicy<Kind>>
    {
        m_workspace.cost_policy.set_action_cost(action_binding, cost);
    }

    datalog::Cost get_predicate_cost(datalog::PredicateAnnotationHead<Kind> head) const noexcept
    {
        const auto* annotation = m_workspace.and_annot.find(head);
        return annotation ? datalog::get_cost(*annotation) : datalog::Cost(0);
    }

    datalog::Cost get_goal_cost() const noexcept
    {
        const auto& numeric_support_selector = m_workspace.get_numeric_support_selector();
        return m_workspace.tp.get_total_cost(datalog::FactSets { m_workspace.const_workspace.facts.fact_sets, m_workspace.facts.fact_sets },
                                             m_workspace.and_annot,
                                             m_workspace.numeric_and_annot,
                                             numeric_support_selector);
    }

    template<::tyr::formalism::RelationKind R>
    auto get_action(const datalog::WitnessAnnotation<Kind, R>& witness)
    {
        return Policy::get_action(*m_definition, m_workspace, witness);
    }

    template<::tyr::formalism::RelationKind R>
    std::optional<::tyr::formalism::planning::ActionBindingView> get_action_binding(const datalog::WitnessAnnotation<Kind, R>& witness)
    {
        const auto action = get_action(witness);
        if (!action)
            return std::nullopt;
        return Policy::get_action_binding(*action);
    }

    template<::tyr::formalism::RelationKind R, typename Callback>
    void for_each_witness_precondition(const datalog::WitnessAnnotation<Kind, R>& witness, Callback&& callback)
    {
        Policy::for_each_witness_precondition(*m_definition,
                                              m_workspace,
                                              witness,
                                              std::forward<Callback>(callback),
                                              [](::tyr::formalism::datalog::GroundBooleanOperatorView) {});
    }

    template<::tyr::formalism::RelationKind R, typename PredicateCallback, typename NumericCallback>
    void for_each_witness_precondition(const datalog::WitnessAnnotation<Kind, R>& witness,
                                       PredicateCallback&& predicate_callback,
                                       NumericCallback&& numeric_callback)
    {
        Policy::for_each_witness_precondition(*m_definition,
                                              m_workspace,
                                              witness,
                                              std::forward<PredicateCallback>(predicate_callback),
                                              std::forward<NumericCallback>(numeric_callback));
    }

    template<typename Callback>
    void for_each_achiever(datalog::PredicateAnnotationHead<Kind> head, Callback&& callback)
        requires AndAP::records_propositional_achievers
    {
        const auto* achievers = m_workspace.and_ap.find_achievers(head);
        if (!achievers)
            return;

        for (const auto& achiever : *achievers)
            callback(achiever);
    }

    Task<Kind>& get_task() noexcept { return *m_definition->task; }
    const Task<Kind>& get_task() const noexcept { return *m_definition->task; }
    const RPGProgram<Kind>& get_program() const noexcept { return m_definition->rpg_program; }

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
