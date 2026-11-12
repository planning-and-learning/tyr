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
#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/policies/annotation_concepts.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/datalog/policies/cost_concepts.hpp"
#include "tyr/datalog/policies/termination_concepts.hpp"
#include "tyr/datalog/solver.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/interning.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/programs/rpg.hpp"
#include "tyr/planning/task_utils.hpp"

#include <concepts>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/execution/onetbb.hpp>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning::detail
{

template<TaskKind Kind>
struct RPGDefinition
{
    TaskPtr<Kind> task;
    RPGProgram<Kind> rpg_program;

    RPGDefinition(TaskPtr<Kind> task_, CostMode cost_mode, bool split_rules = false) :
        task(std::move(task_)),
        rpg_program(this->task->get_task(), cost_mode, split_rules)
    {
    }
};

template<TaskKind Kind>
struct RPGPolicy;

inline formalism::planning::ActionRowView to_action_binding(formalism::planning::ActionView<GroundTag> action) noexcept { return action.get_row(); }

inline formalism::planning::ActionRowView to_action_binding(formalism::planning::ActionRowView action) noexcept { return action; }

template<TaskKind Kind, typename Workspace>
void materialize_goal(RPGDefinition<Kind>& definition, Workspace& workspace, formalism::planning::ConjunctiveConditionView<GroundTag> source_goal)
{
    auto& destination = [&]() -> formalism::datalog::Repository&
    {
        if constexpr (std::same_as<Kind, GroundTag>)
            return definition.rpg_program.get_datalog_program().get_program_repository();
        else
            return *workspace.workspace_repository;
    }();
    auto merge_context = formalism::WriteContext { workspace.datalog_builder, destination };
    auto condition = formalism::datalog::checkout<formalism::datalog::ConjunctiveCondition<GroundTag>>(merge_context.builder());

    const auto translate_atom = [&](const auto atom)
    {
        const auto& p2d = definition.rpg_program.get_translation_context().p2d;
        if constexpr (std::same_as<Kind, GroundTag>)
            return p2d.fluent_to_fluent_atom.at(atom);
        else
            return formalism::planning::merge_p2d(atom, p2d.fluent_to_fluent_predicate, merge_context).first;
    };

    for (const auto fact : source_goal.template get_facts<formalism::PositiveTag>())
    {
        const auto atom = fact.get_atom();
        if (!atom)
            continue;

        auto literal = formalism::datalog::checkout<formalism::Literal<GroundTag, formalism::FluentTag>>(merge_context.builder());
        literal->atom = translate_atom(*atom).index();
        literal->polarity = true;
        condition->fluent_literals.push_back(formalism::datalog::insert(merge_context.destination(), *literal).first.index());
    }

    workspace.tp.set_goals(formalism::datalog::insert(merge_context.destination(), *condition).first);
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

    RPGEvaluator(TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context, CostMode cost_mode = CostMode::UNIT) :
        // h_max, h_add and FF aggregate over the minimal bindings of a rule, so splitting preserves them (LM-cut builds its own definition).
        RPGEvaluator(std::make_shared<Definition>(std::move(task), cost_mode, true), std::move(execution_context))
    {
    }

    RPGEvaluator(std::shared_ptr<Definition> definition, ygg::ExecutionContextPtr execution_context) :
        m_definition(std::move(definition)),
        m_execution_context(std::move(execution_context)),
        m_source_goal(m_definition->task->get_task().get_goal()),
        m_workspace(m_definition->rpg_program.get_datalog_program(), AP {}, TP {}, make_cost_policy(*m_definition))
    { Policy::set_goal(*m_definition, m_workspace, m_source_goal); }

    RPGEvaluator(const RPGEvaluator& source, ygg::ExecutionContextPtr execution_context) :
        m_definition(source.m_definition),
        m_execution_context(std::move(execution_context)),
        m_source_goal(source.m_source_goal),
        m_workspace(m_definition->rpg_program.get_datalog_program(), AP {}, TP {}, make_cost_policy(*m_definition))
    { Policy::set_goal(*m_definition, m_workspace, m_source_goal); }

    void set_goal(formalism::planning::ConjunctiveConditionView<GroundTag> goal)
    {
        m_source_goal = goal;
        Policy::set_goal(*m_definition, m_workspace, m_source_goal);
    }

    ygg::float_t evaluate(const urd::data<State<Kind>>& state)
    {
        begin_state_evaluation();
        return evaluate_current_state(state);
    }

    void print_summary(size_t verbosity) const { Policy::print_summary(m_workspace, verbosity); }

protected:
    void begin_state_evaluation() { Policy::begin_state_evaluation(*m_definition, m_workspace, m_source_goal); }

    ygg::float_t evaluate_current_state(const urd::data<State<Kind>>& state)
    {
        const auto& repository = *m_definition->task->get_repository();
        const auto& translation_context = m_definition->rpg_program.get_translation_context().p2d;
        // LM-cut repeats this evaluation while retaining the current goal and cost overrides.
        m_workspace.facts.reset();
        insert_unextended_state(state, repository, translation_context, m_workspace);
        auto ctx = datalog::ProgramExecutionContext(m_workspace);
        datalog::execute_model(ctx, *m_execution_context);

        return m_workspace.tp.check(datalog::FactSets { m_workspace.const_workspace->facts.fact_sets, m_workspace.facts.fact_sets }) ?
                   static_cast<Derived&>(*this).compute_result(state) :
                   std::numeric_limits<ygg::float_t>::infinity();
    }

    ygg::float_t compute_result(const urd::data<State<Kind>>&) const { return get_goal_cost(); }

    void set_action_binding_cost(formalism::planning::ActionRowView action_binding, datalog::Cost cost)
        requires std::same_as<CP, datalog::RuleCostOverridePolicy<Kind>>
    { m_workspace.cost_policy.set_action_cost(action_binding, cost); }

    [[nodiscard]] datalog::Cost get_predicate_cost(formalism::datalog::PredicateRowView<formalism::FluentTag> head) const noexcept
    {
        const auto* annotation = m_workspace.annotations.find(head.data());
        return annotation ? datalog::get_cost(*annotation) : static_cast<datalog::Cost>(0);
    }

    [[nodiscard]] datalog::Cost get_goal_cost() const
    {
        return m_workspace.tp.get_total_cost(datalog::FactSets { m_workspace.const_workspace->facts.fact_sets, m_workspace.facts.fact_sets },
                                             m_workspace.annotations);
    }

    /// Whether the witness instantiates an action rule.
    [[nodiscard]] bool is_action_witness(const datalog::WitnessAnnotation& witness) const { return Policy::is_action_witness(*m_definition, witness); }

    /// Calls back the binding of every action whose rule the witness instantiates; identical relaxed rules share one rule.
    template<typename Callback>
    void for_each_action_binding(const datalog::WitnessAnnotation& witness, Callback callback)
    { Policy::for_each_action_binding(*m_definition, m_workspace, witness, callback); }

    /// The binding of one action whose rule the witness instantiates, if any.
    std::optional<formalism::planning::ActionRowView> get_action_binding(const datalog::WitnessAnnotation& witness)
    {
        auto result = std::optional<formalism::planning::ActionRowView> {};
        for_each_action_binding(witness,
                                [&](const auto action_binding)
                                {
                                    if (!result)
                                        result = action_binding;
                                });
        return result;
    }

    void append_planning_cut_frontier_atom(formalism::datalog::PredicateRowView<formalism::FluentTag> head,
                                           formalism::planning::AtomViewList<GroundTag, formalism::FluentTag>& atoms)
    {
        if (const auto atom = Policy::translate_cut_atom(*m_definition, m_workspace, head))
            atoms.push_back(*atom);
    }

    template<typename Callback>
    void for_each_witness_precondition(const datalog::WitnessAnnotation& witness, Callback callback)
    {
        Policy::visit_witness_rule_instance(m_workspace,
                                            witness,
                                            [&](const auto& instance)
                                            {
                                                for (const auto literal : instance.get_body().template get_literals<formalism::FluentTag>())
                                                    if (literal.get_polarity())
                                                        callback(instance.resolve(literal.get_atom()));
                                            });
    }

    template<typename Callback>
    void for_each_achiever(formalism::datalog::PredicateRowView<formalism::FluentTag> head, Callback callback)
        requires AP::records_propositional_achievers
    {
        const auto* achievers = m_workspace.annotation_policy.find_achievers(head.data());
        if (!achievers)
            return;

        for (const auto& achiever : *achievers)
            callback(achiever);
    }

    const Task<Kind>& get_task() const noexcept { return *m_definition->task; }

    Workspace& workspace() noexcept { return m_workspace; }
    const Workspace& workspace() const noexcept { return m_workspace; }

private:
    std::shared_ptr<Definition> m_definition;
    ygg::ExecutionContextPtr m_execution_context;
    formalism::planning::ConjunctiveConditionView<GroundTag> m_source_goal;
    Workspace m_workspace;

    static CP make_cost_policy(const Definition& definition)
    {
        if constexpr (std::same_as<CP, datalog::RuleCostOverridePolicy<Kind>>)
            return CP(definition.rpg_program.get_rule_to_action_mapping());
        else
            return CP {};
    }
};

}

#endif
