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

#ifndef TYR_PLANNING_LIFTED_PROGRAMS_RPG_HPP_
#define TYR_PLANNING_LIFTED_PROGRAMS_RPG_HPP_

#include "tyr/datalog/lifted/programs/program.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/lifted/programs/translation_context.hpp"
#include "tyr/planning/programs/rpg.hpp"

#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::planning
{

template<>
class RPGProgram<LiftedTag>
{
public:
    template<::tyr::formalism::RelationKind R>
    using RuleToActionMapping = ygg::UnorderedMap<::tyr::formalism::datalog::RuleView<R>, ::tyr::formalism::planning::ActionView>;
    struct RuleToActionMappings
    {
        RuleToActionMapping<::tyr::formalism::PredicateTag> predicate;
        RuleToActionMapping<::tyr::formalism::FunctionTag> function;
    };

    explicit RPGProgram(::tyr::formalism::planning::TaskView task, CostMode cost_mode = CostMode::GENERAL);

    const TranslationContext<LiftedTag>& get_translation_context() const noexcept;
    template<::tyr::formalism::RelationKind R>
    const RuleToActionMapping<R>& get_rule_to_action_mapping() const noexcept;
    datalog::Program<LiftedTag>& get_datalog_program() noexcept;
    const datalog::Program<LiftedTag>& get_datalog_program() const noexcept;
    ::tyr::formalism::datalog::GroundConjunctiveConditionView get_goal() const noexcept;

private:
    TranslationContext<LiftedTag> m_translation_context;
    RuleToActionMappings m_rule_to_action;
    datalog::Program<LiftedTag> m_datalog_program;
};

}

#endif
