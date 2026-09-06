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

#ifndef TYR_PLANNING_PROGRAMS_RPG_HPP_
#define TYR_PLANNING_PROGRAMS_RPG_HPP_

#include "tyr/datalog/ground/program.hpp"
#include "tyr/datalog/lifted/program.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/ground/programs/translation_context.hpp"
#include "tyr/planning/lifted/programs/translation_context.hpp"

#include <concepts>
#include <type_traits>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
class RPGProgram
{
public:
    using Task = std::conditional_t<std::same_as<Kind, GroundTag>, formalism::planning::FDRTaskView, formalism::planning::TaskView>;
    using Action = std::conditional_t<std::same_as<Kind, GroundTag>, formalism::planning::ActionView<GroundTag>, formalism::planning::ActionView<LiftedTag>>;
    template<formalism::RelationKind R>
    using Rule = std::conditional_t<std::same_as<Kind, GroundTag>, formalism::datalog::RuleBindingView<R>, formalism::datalog::RuleView<LiftedTag, R>>;
    template<formalism::RelationKind R>
    using RuleToActionMapping = ygg::UnorderedMap<Rule<R>, Action>;

    struct RuleToActionMappings
    {
        RuleToActionMapping<formalism::PredicateTag> predicate;
        RuleToActionMapping<formalism::FunctionTag> function;
    };

    explicit RPGProgram(Task task, CostMode cost_mode = CostMode::GENERAL);

    const TranslationContext<Kind>& get_translation_context() const noexcept { return m_translation_context; }

    template<formalism::RelationKind R>
    const RuleToActionMapping<R>& get_rule_to_action_mapping() const noexcept
    {
        if constexpr (std::same_as<R, formalism::PredicateTag>)
            return m_rule_to_action.predicate;
        else
            return m_rule_to_action.function;
    }

    datalog::Program<Kind>& get_datalog_program() noexcept { return m_datalog_program; }
    const datalog::Program<Kind>& get_datalog_program() const noexcept { return m_datalog_program; }
    formalism::datalog::ConjunctiveConditionView<GroundTag> get_goal() const noexcept { return m_datalog_program.get_program().get_goal().value(); }

private:
    TranslationContext<Kind> m_translation_context;
    RuleToActionMappings m_rule_to_action;
    datalog::Program<Kind> m_datalog_program;
};

template<>
RPGProgram<GroundTag>::RPGProgram(formalism::planning::FDRTaskView task, CostMode cost_mode);

template<>
RPGProgram<LiftedTag>::RPGProgram(formalism::planning::TaskView task, CostMode cost_mode);

}

#endif
