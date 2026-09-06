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
#include "tyr/datalog/ground/rule_instance.hpp"
#include "tyr/datalog/ground/workspaces/program.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/planning/ground/state_builder.hpp"
#include "tyr/planning/ground/task.hpp"
#include "tyr/planning/programs/rpg.hpp"

#include <optional>
#include <stdexcept>
#include <utility>

namespace tyr::planning::detail
{

template<>
struct RPGPolicy<GroundTag>
{
    using Action = formalism::planning::ActionView<GroundTag>;

    template<typename Workspace>
    static std::optional<formalism::planning::AtomView<GroundTag, formalism::FluentTag>>
    translate_cut_atom(const RPGDefinition<GroundTag>& definition,
                       Workspace&,
                       formalism::datalog::PredicateBindingView<formalism::FluentTag> head)
    {
        const auto& mapping = definition.rpg_program.get_translation_context().d2p.fluent_to_fluent_atom;
        if (const auto it = mapping.find(head); it != mapping.end())
            return it->second;
        return std::nullopt;
    }

    template<typename Workspace>
    static void set_goal(RPGDefinition<GroundTag>& definition, Workspace& workspace, formalism::planning::ConjunctiveConditionView<GroundTag> source_goal)
    {
        materialize_goal(definition, workspace, source_goal);
    }

    template<typename Workspace>
    static void begin_state_evaluation(const RPGDefinition<GroundTag>&, Workspace& workspace, formalism::planning::ConjunctiveConditionView<GroundTag>)
    {
        workspace.clear_costs();
    }

    template<formalism::RelationKind R, typename Workspace>
    static std::optional<Action> get_action(const RPGDefinition<GroundTag>& definition, Workspace&, const datalog::WitnessAnnotation<R>& witness)
    {
        const auto& mapping = definition.rpg_program.template get_rule_to_action_mapping<R>();
        const auto it = mapping.find(witness.get_rule_key());
        return it == mapping.end() ? std::nullopt : std::optional(it->second);
    }

    template<typename Workspace, typename Executor>
    static bool
    is_action_applicable(const RPGDefinition<GroundTag>&, Workspace&, Executor& executor, Action action, const StateContext<GroundTag>& state_context)
    {
        return executor.is_applicable(action, state_context);
    }

    template<formalism::RelationKind R, typename Workspace, typename Callback>
    static void visit_witness_rule_instance(Workspace&, const datalog::WitnessAnnotation<R>& witness, Callback&& callback)
    {
        const auto rule = formalism::datalog::find_ground_rule(witness.get_rule_key());
        if (!rule)
            throw std::logic_error("Ground witness binding has no rule");

        const auto instance = datalog::RuleInstance<GroundTag, R>(*rule);
        std::forward<Callback>(callback)(instance);
    }

    template<typename Workspace>
    static void print_summary(const Workspace&, size_t) noexcept
    {
    }
};

}

#endif
