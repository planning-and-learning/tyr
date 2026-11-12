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

#ifndef TYR_SRC_PLANNING_LIFTED_HEURISTICS_RPG_HPP_
#define TYR_SRC_PLANNING_LIFTED_HEURISTICS_RPG_HPP_

#include "../../heuristics/rpg.hpp"
#include "tyr/datalog/formatter.hpp"
#include "tyr/datalog/lifted/contexts/program.hpp"
#include "tyr/datalog/lifted/rule_instance.hpp"
#include "tyr/datalog/lifted/workspaces/program.hpp"
#include "tyr/datalog/policies/cost.hpp"
#include "tyr/formalism/datalog/grounder.hpp"
#include "tyr/formalism/planning/grounder.hpp"
#include "tyr/formalism/planning/merge_planning.hpp"
#include "tyr/planning/lifted/state_storage/vectors.hpp"
#include "tyr/planning/lifted/task.hpp"
#include "tyr/planning/programs/rpg.hpp"
#include "tyr/planning/state_builder.hpp"
#include "tyr/planning/state_data.hpp"
#include "tyr/planning/state_repository.hpp"
#include "tyr/planning/state_storage/numeric.hpp"
#include "tyr/planning/state_storage/tree_bitset/atom.hpp"
#include "tyr/planning/state_storage/tree_bitset/fact.hpp"

#include <cassert>
#include <fmt/ostream.h>
#include <optional>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::planning::detail
{

template<>
struct RPGPolicy<LiftedTag>
{
    using Action = formalism::planning::ActionRowView;

    template<typename Workspace>
    static std::optional<formalism::planning::AtomView<GroundTag, formalism::FluentTag>>
    translate_cut_atom(const RPGDefinition<LiftedTag>& definition, Workspace& workspace, formalism::datalog::PredicateRowView<formalism::FluentTag> head)
    {
        const auto& mapping = definition.rpg_program.get_translation_context().d2p.fluent_to_fluent_predicate;
        if (!mapping.contains(head.get_relation()))
            return std::nullopt;

        auto merge_context = formalism::WriteContext { workspace.planning_builder, *definition.task->get_repository() };
        return formalism::planning::merge_atom_d2p<formalism::FluentTag, formalism::FluentTag>(head, mapping, merge_context).first;
    }

    template<typename Workspace>
    static void set_goal(RPGDefinition<LiftedTag>&, Workspace&, formalism::planning::ConjunctiveConditionView<GroundTag>) noexcept
    {
        // Lifted ground bindings live in the evaluation repository, which reset_evaluation()
        // clears. The goal is therefore materialized in begin_state_evaluation() after the reset.
    }

    template<typename Workspace>
    static void
    begin_state_evaluation(RPGDefinition<LiftedTag>& definition, Workspace& workspace, formalism::planning::ConjunctiveConditionView<GroundTag> source_goal)
    {
        workspace.reset_evaluation();
        // Lifted ground bindings live in the evaluation repository and must be restored after it is cleared.
        materialize_goal(definition, workspace, source_goal);
    }

    static bool is_action_witness(const RPGDefinition<LiftedTag>& definition, const datalog::WitnessAnnotation& witness)
    { return definition.rpg_program.get_rule_to_action_mapping().contains(witness.get_rule_key().relation); }

    /// Grounds every action of the witness's rule with the binding the witness recovers.
    template<typename Workspace, typename Callback>
    static void
    for_each_action_binding(const RPGDefinition<LiftedTag>& definition, Workspace& workspace, const datalog::WitnessAnnotation& witness, Callback callback)
    {
        const auto& mapping = definition.rpg_program.get_rule_to_action_mapping();
        const auto it = mapping.find(witness.get_rule_key().relation);
        if (it == mapping.end())
            return;

        auto grounder_context = formalism::WriteContext { workspace.planning_builder, *definition.task->get_repository(), workspace.binding };
        workspace.binding.clear();
        auto recovered = false;
        const auto& origins = definition.rpg_program.get_rule_splitting().origins;
        if (const auto origin = origins.find(witness.get_rule_key().relation); origin != origins.end())
        {
            workspace.binding.resize(origin->second.num_source_variables);
            recover_split_binding(definition, workspace, witness);
            recovered = true;
        }
        if (!recovered)
        {
            const auto rule_binding = urd::resolve(witness.get_rule_key(), *workspace.workspace_repository);
            for (const auto object : rule_binding.template get<urd::index<formalism::Object>>())
                workspace.binding.push_back(object);
        }
        for (const auto action : it->second)
            callback(formalism::planning::ground(action, grounder_context).first);
    }

    /// Writes the objects that the witness of a split rule binds to its variables' positions among the source rule's variables,
    /// and those of the projections it uses, through the witnesses of their atoms.
    template<typename Workspace>
    static void recover_split_binding(const RPGDefinition<LiftedTag>& definition, Workspace& workspace, const datalog::WitnessAnnotation& witness)
    {
        const auto& origin = definition.rpg_program.get_rule_splitting().origins.at(witness.get_rule_key().relation);
        const auto rule_binding = urd::resolve(witness.get_rule_key(), *workspace.workspace_repository);
        auto position = static_cast<size_t>(0);
        for (const auto object : rule_binding.template get<urd::index<formalism::Object>>())
            workspace.binding[origin.source_positions[position++]] = object;

        const auto& projection_rules = definition.rpg_program.get_rule_splitting().projection_rules;
        visit_witness_rule_instance(workspace,
                                    witness,
                                    [&](const auto& instance)
                                    {
                                        for (const auto literal : instance.get_body().template get_literals<formalism::FluentTag>())
                                        {
                                            if (!projection_rules.contains(literal.get_atom().get_predicate().index()))
                                                continue;
                                            const auto* annotation = workspace.annotations.find(instance.resolve(literal.get_atom()).data());
                                            const auto* projection = annotation ? std::get_if<datalog::WitnessAnnotation>(annotation) : nullptr;
                                            assert(projection && "A derived projection atom has a witness.");
                                            if (projection)
                                                recover_split_binding(definition, workspace, *projection);
                                        }
                                    });
    }

    template<typename Workspace, typename Callback>
    static void visit_witness_rule_instance(Workspace& workspace, const datalog::WitnessAnnotation& witness, Callback&& callback)
    {
        const auto rule_binding = urd::resolve(witness.get_rule_key(), *workspace.workspace_repository);
        const auto& const_rule_workspace = *workspace.const_workspace->engine.rules[static_cast<ygg::uint_t>(witness.get_rule_key().relation.value)];
        auto binding = urd::flat_vector<urd::index<formalism::Object>> {};
        for (const auto object : rule_binding.template get<urd::index<formalism::Object>>())
            binding.push_back(object);
        auto grounder_context = formalism::WriteContext { workspace.datalog_builder, *workspace.workspace_repository, binding };
        const auto instance = datalog::RuleInstance<LiftedTag>(const_rule_workspace.get_rule(), grounder_context);
        std::forward<Callback>(callback)(instance);
    }

    template<typename Workspace>
    static void print_summary(const Workspace& workspace, size_t verbosity)
    {
        if (verbosity < 1)
            return;

        std::cout << "[RPGHeuristic] Summary" << '\n';
        fmt::print(std::cout, "{}\n", workspace.statistics);
        datalog::print_rule_statistics(workspace.engine.rules);
    }
};

}

#endif
