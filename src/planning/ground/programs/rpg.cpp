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

#include "tyr/planning/programs/rpg.hpp"

#include "../../programs/common.hpp"
#include "tyr/datalog/applicability.hpp"
#include "tyr/datalog/static_rule_filter.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/grounding_context.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/repository.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <optional>
#include <stdexcept>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/slots/slots.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/store.hpp>
#include <utility>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{
namespace
{
struct GroundProgramBuildContext
{
    formalism::Builder builder;
    formalism::GroundingContext<formalism::Builder, formalism::Repository> grounding_context;
    urd::handle<urd::builder<formalism::Program<GroundTag>, formalism::Repository>> program;

    explicit GroundProgramBuildContext(formalism::Repository& repository) :
        builder(),
        grounding_context(builder, repository),
        program(formalism::checkout<formalism::Program<GroundTag>>(builder))
    {
    }
};

void fill_delete_free_condition(formalism::ConjunctiveConditionView<GroundTag> condition,
                                GroundProgramBuildContext& context,
                                urd::builder<formalism::RuleBody<GroundTag>, formalism::Repository>& result)
{
    for (const auto fact : condition.template get_facts<formalism::PositiveTag>())
        if (const auto literal = create_fact_literal(fact, true, context.grounding_context))
            urd::append(result.fluent_literals, *literal);
}

formalism::RuleBodyView<GroundTag> create_delete_free_condition(formalism::ConjunctiveConditionView<GroundTag> condition, GroundProgramBuildContext& context)
{
    auto result = formalism::checkout<formalism::RuleBody<GroundTag>>(context.builder);
    fill_delete_free_condition(condition, context, *result);

    return context.grounding_context.destination().insert(*result, context.builder).first;
}

/// The body of a conditional effect's rule: the action's precondition and the effect's condition, delete-free.
formalism::RuleBodyView<GroundTag> create_delete_free_effect_condition(formalism::ConjunctiveConditionView<GroundTag> action_condition,
                                                                       formalism::ConjunctiveConditionView<GroundTag> effect_condition,
                                                                       GroundProgramBuildContext& context)
{
    auto result = formalism::checkout<formalism::RuleBody<GroundTag>>(context.builder);
    fill_delete_free_condition(action_condition, context, *result);
    fill_delete_free_condition(effect_condition, context, *result);
    return context.grounding_context.destination().insert(*result, context.builder).first;
}

/// Identical rules are one rule: the second is not inserted.
std::pair<formalism::RuleView<GroundTag>, bool>
create_ground_atom_rule(formalism::RuleBodyView<GroundTag> body,
                        const urd::flat_vector<urd::index<formalism::Atom<GroundTag, formalism::FluentTag>>>& heads,
                        GroundProgramBuildContext& context,
                        ygg::float_t cost = 0)
{
    auto rule = formalism::checkout<formalism::Rule<GroundTag>>(context.builder);
    urd::store(rule->body, body);
    urd::build_member(rule->fluent_heads, heads, context.grounding_context.destination());
    rule->cost = cost;
    return context.grounding_context.destination().insert(*rule, context.builder);
}

void translate_action_to_delete_free_rules(formalism::ActionView<GroundTag> action,
                                           urd::builder<formalism::Program<GroundTag>, formalism::Repository>& program,
                                           GroundProgramBuildContext& context,
                                           RPGProgram<GroundTag>::RuleToActionMapping& rule_to_action)
{
    // One rule per conditional effect, its body the action's precondition and the effect's condition: no intermediate
    // applicability atom, so an applicable action fires once per effect.
    for (const auto cond_eff : action.get_effects())
    {
        const auto body = create_delete_free_effect_condition(action.get_condition(), cond_eff.get_condition(), context);

        auto heads = urd::flat_vector<urd::index<formalism::Atom<GroundTag, formalism::FluentTag>>> {};
        for (const auto fact : cond_eff.get_effect().get_facts<formalism::PositiveTag>())
            if (const auto atom = fact.get_atom())
                heads.push_back(atom->index());
        if (!heads.empty())
        {
            const auto [rule, inserted] = create_ground_atom_rule(body, heads, context, 1);
            if (inserted)
                urd::append(program.rules, rule);
            rule_to_action[rule.index()].push_back(action);
        }
    }
}

formalism::ProgramView<GroundTag>
create_rpg_ground_program(formalism::TaskView<GroundTag> task, RPGProgram<GroundTag>::RuleToActionMapping& mapping, formalism::Repository& repository)
{
    auto context = GroundProgramBuildContext(repository);

    translate_task_symbols(task, *context.program);
    // The atoms of the fluent facts too, each once.
    auto fluent_atoms = urd::Store<urd::set<formalism::AtomView<GroundTag, formalism::FluentTag>>> {};
    for (const auto atom : task.get_atoms<formalism::FluentTag>())
        fluent_atoms.insert(atom);
    for (const auto fact : task.get_fluent_facts())
        if (const auto atom = fact.get_atom(); atom && fluent_atoms.insert(*atom).second)
            urd::append(context.program->fluent_atoms, *atom);

    urd::store(context.program->goal, create_delete_free_condition(task.get_goal(), context));

    for (const auto action : task.get_ground_actions())
        translate_action_to_delete_free_rules(action, *context.program, context, mapping);

    return repository.insert(*context.program, context.builder).first;
}

/// The actions of the rules the static rule filter keeps: the filter's layer holds the source's rules as they are.
void filter_rule_to_action(const RPGProgram<GroundTag>::RuleToActionMapping& source_mapping,
                           RPGProgram<GroundTag>::RuleToActionMapping& result_mapping,
                           const formalism::Repository& source_repository,
                           const datalog::FactSets& fact_sets)
{
    for (const auto& [source_index, actions] : source_mapping)
        if (datalog::is_statically_applicable(urd::resolve(source_index, source_repository), fact_sets))
            result_mapping[source_index].insert(result_mapping[source_index].end(), actions.begin(), actions.end());
}

datalog::Program<GroundTag>
create_rpg_datalog_program(const formalism::PlanningTask<GroundTag>& planning_task, CostMode cost_mode, RPGProgram<GroundTag>::RuleToActionMapping& mapping)
{
    require_unit_cost(cost_mode);

    const auto task = planning_task.get_task();
    auto source_repository = layer_over(planning_task.get_repository());
    auto source_mapping = RPGProgram<GroundTag>::RuleToActionMapping {};
    const auto source_program = create_rpg_ground_program(task, source_mapping, *source_repository);
    const auto source_static_fact_sets = datalog::TaggedFactSets<formalism::StaticTag>(source_program.get_predicates<formalism::StaticTag>(),
                                                                                       source_program.get_functions<formalism::StaticTag>(),
                                                                                       source_program.get_atoms<formalism::StaticTag>(),
                                                                                       source_program.get_fterm_values<formalism::StaticTag>(),
                                                                                       source_program.context());
    auto source_mutable_fact_sets = datalog::MutableFactSets(source_program, source_program.context());
    datalog::insert_program_facts(source_mutable_fact_sets, source_program);
    const auto source_fact_sets = datalog::FactSets { source_static_fact_sets, source_mutable_fact_sets };

    auto repository = layer_over(source_repository);
    const auto program = datalog::remove_statically_inapplicable_rules(source_program, *repository);
    filter_rule_to_action(source_mapping, mapping, *source_repository, source_fact_sets);
    return datalog::Program<GroundTag>(program, std::move(repository));
}

}  // namespace

template<>
RPGProgram<GroundTag>::RPGProgram(const formalism::PlanningTask<GroundTag>& task, CostMode cost_mode, bool) :
    m_rule_to_action(),
    m_datalog_program(create_rpg_datalog_program(task, cost_mode, m_rule_to_action))
{
}

}  // namespace tyr::planning
