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
#include "tyr/planning/ground/task.hpp"

#include <optional>

namespace tyr::planning::detail
{

template<>
struct RPGPolicy<GroundTag>
{
    using Action = ::tyr::formalism::planning::GroundActionView;
    using PredicateHead = datalog::PredicateAnnotationHead<GroundTag>;

    static PredicateHead get_predicate_head(::tyr::formalism::datalog::GroundAtomView<::tyr::formalism::FluentTag> atom) noexcept { return atom; }
    static auto get_predicate_binding(PredicateHead head) noexcept { return head.get_row(); }

    template<typename Definition, typename Workspace>
    static void append_cut_frontier_atom(const Definition& definition,
                                         Workspace&,
                                         PredicateHead head,
                                         ::tyr::formalism::planning::GroundAtomViewList<::tyr::formalism::FluentTag>& atoms)
    {
        const auto& mapping = definition.rpg_program.get_translation_context().d2p.fluent_to_fluent_atom;
        if (const auto it = mapping.find(head); it != mapping.end())
            atoms.push_back(it->second);
    }

    template<typename Definition, typename Workspace>
    static void set_goal(Definition& definition, Workspace& workspace, ::tyr::formalism::planning::GroundConjunctiveConditionView source_goal)
    {
        namespace fd = ::tyr::formalism::datalog;
        auto builder = fd::Builder {};
        auto& repository = definition.rpg_program.get_datalog_program().get_program_repository();
        auto merge_context = ::tyr::formalism::planning::MergeDatalogContext { builder, repository };
        const auto& p2d = definition.rpg_program.get_translation_context().p2d.fluent_to_fluent_atom;
        materialize_goal(workspace, source_goal, merge_context, [&](const auto atom) { return p2d.at(atom); });
    }

    template<typename Definition, typename Workspace>
    static void begin_state_evaluation(const Definition&, Workspace& workspace, ::tyr::formalism::planning::GroundConjunctiveConditionView)
    {
        workspace.clear_costs();
    }

    template<::tyr::formalism::RelationKind R, typename Definition, typename Workspace>
    static std::optional<Action> get_action(const Definition& definition, Workspace&, const datalog::WitnessAnnotation<GroundTag, R>& witness)
    {
        const auto& mapping = definition.rpg_program.template get_rule_to_action_mapping<R>();
        const auto it = mapping.find(witness.get_rule_key());
        return it == mapping.end() ? std::nullopt : std::optional(it->second);
    }

    static ::tyr::formalism::planning::ActionBindingView get_action_binding(Action action) noexcept { return action.get_row(); }

    template<typename Workspace, typename Callback>
    static void for_each_numeric_predecessor(Workspace&, const datalog::NumericSupport<GroundTag>& support, Callback&& callback)
    {
        callback(support.get_key(), support.get_interval(), support.get_cost());
    }

    template<typename Definition, typename Workspace, typename Executor>
    static bool is_action_applicable(const Definition&, Workspace&, Executor& executor, Action action, const StateContext<GroundTag>& state_context)
    {
        return executor.is_applicable(action, state_context);
    }

    template<::tyr::formalism::RelationKind R, typename Definition, typename Workspace, typename PredicateCallback, typename NumericCallback>
    static void for_each_witness_precondition(const Definition&,
                                              Workspace&,
                                              const datalog::WitnessAnnotation<GroundTag, R>& witness,
                                              PredicateCallback&& predicate_callback,
                                              NumericCallback&& numeric_callback)
    {
        const auto body = witness.get_rule_key().get_body();
        for (const auto literal : body.template get_literals<::tyr::formalism::FluentTag>())
            if (literal.get_polarity())
                predicate_callback(literal.get_atom());

        for (const auto constraint : body.get_numeric_constraints())
            numeric_callback(constraint);
    }

    template<typename Workspace>
    static void print_summary(const Workspace&, size_t) noexcept
    {
    }
};

}

#endif
