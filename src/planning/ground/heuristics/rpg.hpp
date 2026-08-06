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

#ifndef TYR_SRC_PLANNING_GROUND_HEURISTICS_RPG_HPP_
#define TYR_SRC_PLANNING_GROUND_HEURISTICS_RPG_HPP_

#include "../../heuristics/rpg.hpp"
#include "tyr/datalog/ground/contexts/program.hpp"
#include "tyr/datalog/ground/workspaces/program.hpp"
#include "tyr/formalism/datalog/builder.hpp"
#include "tyr/formalism/datalog/canonicalization.hpp"
#include "tyr/formalism/planning/merge_datalog.hpp"
#include "tyr/planning/ground/programs/rpg.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/state_iterators.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/state_iterators.hpp"

#include <optional>

namespace tyr::planning::detail
{

template<>
struct RPGPolicy<GroundTag>
{
    template<typename Definition, typename Workspace>
    static void set_goal(Definition& definition, Workspace& workspace, ::tyr::formalism::planning::GroundConjunctiveConditionView source_goal)
    {
        namespace fd = ::tyr::formalism::datalog;
        auto builder = fd::Builder {};
        auto& repository = definition.rpg_program.get_datalog_program().get_program_repository();
        auto merge_context = ::tyr::formalism::planning::MergeDatalogContext { builder, repository };
        auto condition_ptr = builder.get_builder<fd::GroundConjunctiveCondition>();
        auto& condition = *condition_ptr;
        condition.clear();

        const auto& p2d = definition.rpg_program.get_translation_context().p2d.fluent_to_fluent_atom;
        for (const auto fact : source_goal.template get_facts<::tyr::formalism::PositiveTag>())
        {
            if (const auto atom = fact.get_atom())
            {
                auto literal_ptr = builder.get_builder<fd::GroundLiteral<::tyr::formalism::FluentTag>>();
                auto& literal = *literal_ptr;
                literal.clear();
                literal.atom = p2d.at(*atom).get_index();
                literal.polarity = true;
                fd::canonicalize(literal);
                condition.fluent_literals.push_back(repository.get_or_create(literal).first.get_index());
            }
        }

        for (const auto numeric_constraint : source_goal.get_numeric_constraints())
            condition.numeric_constraints.push_back(::tyr::formalism::planning::merge_p2d(numeric_constraint, merge_context));

        fd::canonicalize(condition);
        workspace.tp.set_goals(repository.get_or_create(condition).first);
    }

    template<typename Definition, typename Workspace>
    static void begin_state_evaluation(const Definition&, Workspace& workspace, ::tyr::formalism::planning::GroundConjunctiveConditionView)
    {
        workspace.clear_costs();
    }

    template<typename Definition, typename Workspace>
    static void insert_state_facts(const Definition& definition, Workspace& workspace, const ygg::Builder<State<GroundTag>>& state)
    {
        const auto& p2d = definition.rpg_program.get_translation_context().p2d;
        const auto& repository = *definition.task->get_repository();
        const auto fluent_facts = FDRFactRange<GroundTag, ::tyr::formalism::FluentTag>(state.template get_atoms<::tyr::formalism::FluentTag>().values);
        for (const auto fact : fluent_facts)
        {
            const auto fact_view = ygg::make_view(fact, repository);
            if (const auto atom = fact_view.get_atom())
                workspace.facts.fact_sets.predicate.insert(p2d.fluent_to_fluent_atom.at(*atom));
        }

        const auto fluent_fterm_values = FunctionTermValueRange<::tyr::formalism::FluentTag>(state.get_numeric_variables().values);
        for (const auto& [fterm_index, value] : fluent_fterm_values)
        {
            const auto fterm = ygg::make_view(fterm_index, repository);
            if (const auto it = p2d.fluent_to_fluent_fterm.find(fterm); it != p2d.fluent_to_fluent_fterm.end())
                workspace.facts.fact_sets.function.insert(it->second, value);
        }
    }

    template<typename Workspace>
    static void prepare_rule_binding(Workspace&, ::tyr::formalism::planning::ActionBindingView) noexcept
    {
    }

    template<::tyr::formalism::RelationKind R, typename Workspace>
    static std::optional<datalog::WitnessRuleKeyT<GroundTag, R>> make_rule_cost_key(Workspace&,
                                                                                    ::tyr::formalism::datalog::GroundRuleView<R> rule,
                                                                                    ::tyr::formalism::planning::GroundActionView mapped_action,
                                                                                    ::tyr::formalism::planning::ActionBindingView action_binding)
    {
        return mapped_action.get_row() == action_binding ? std::optional(rule) : std::nullopt;
    }

    template<::tyr::formalism::RelationKind R, typename Definition, typename Workspace>
    static std::optional<::tyr::formalism::planning::ActionBindingView>
    get_action_binding(const Definition& definition, Workspace&, const datalog::WitnessAnnotation<GroundTag, R>& witness)
    {
        const auto& mapping = definition.rpg_program.template get_rule_to_action_mapping<R>();
        const auto it = mapping.find(witness.get_rule_key());
        return it == mapping.end() ? std::nullopt : std::optional(it->second.get_row());
    }

    template<::tyr::formalism::RelationKind R, typename Definition, typename Workspace, typename Callback>
    static void for_each_witness_precondition(const Definition&, Workspace&, const datalog::WitnessAnnotation<GroundTag, R>& witness, Callback&& callback)
    {
        for (const auto literal : witness.get_rule_key().get_body().template get_literals<::tyr::formalism::FluentTag>())
            if (literal.get_polarity())
                callback(literal.get_atom());
    }

    template<typename Workspace>
    static void print_summary(const Workspace&, size_t) noexcept
    {
    }
};

}

#endif
