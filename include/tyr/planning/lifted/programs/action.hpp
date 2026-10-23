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

#ifndef TYR_PLANNING_LIFTED_PROGRAMS_ACTION_HPP_
#define TYR_PLANNING_LIFTED_PROGRAMS_ACTION_HPP_

#include "tyr/datalog/lifted/program.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/lifted/programs/translation_context.hpp"
#include "tyr/planning/programs/action.hpp"

#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::planning
{

template<>
class ApplicableActionProgram<LiftedTag>
{
public:
    using AppPredicateToActionMapping =
        ygg::UnorderedMap<formalism::datalog::PredicateView<formalism::FluentTag>, formalism::planning::ActionView<LiftedTag>>;

    struct SchemaProgram
    {
        SchemaProgram(formalism::datalog::ProgramView<LiftedTag> program, const P2DTranslationContext<LiftedTag>& translation_context);

        formalism::datalog::ProgramView<LiftedTag> program;
        P2DTranslationContext<LiftedTag> input_translation;
        analysis::RuleStrata strata;
        analysis::ListenerStrata listeners;
    };

    /// A program per action schema with its action rule and the rules of its conditional effects.
    using SchemaPrograms = ygg::UnorderedMap<formalism::planning::ActionView<LiftedTag>, SchemaProgram>;

    /// The conditional effect of an effect predicate, whose atoms are the objects of an applicable binding of the action followed
    /// by objects of the effect's variables for which the effect fires. Effects without variables and condition have none.
    struct Effect
    {
        formalism::planning::ActionView<LiftedTag> action;
        size_t position;  ///< Among the action's effects.
    };
    using EffectPredicateToEffectMapping = ygg::UnorderedMap<formalism::datalog::PredicateView<formalism::FluentTag>, Effect>;

    explicit ApplicableActionProgram(formalism::planning::TaskView<LiftedTag> task);

    const TranslationContext<LiftedTag>& get_translation_context() const noexcept;
    const AppPredicateToActionMapping& get_predicate_to_action_mapping() const noexcept;
    const SchemaPrograms& get_schema_programs() const noexcept;
    const EffectPredicateToEffectMapping& get_effect_predicate_to_effect_mapping() const noexcept;
    datalog::Program<LiftedTag>& get_datalog_program() noexcept;
    const datalog::Program<LiftedTag>& get_datalog_program() const noexcept;
    const datalog::ConstProgramWorkspace<LiftedTag>& get_const_program_workspace() const noexcept;

private:
    TranslationContext<LiftedTag> m_translation_context;
    AppPredicateToActionMapping m_predicate_to_actions;
    SchemaPrograms m_schema_programs;
    EffectPredicateToEffectMapping m_effect_predicate_to_effect;
    datalog::Program<LiftedTag> m_datalog_program;
};

}

#endif
