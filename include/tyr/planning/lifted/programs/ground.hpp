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

#ifndef TYR_PLANNING_LIFTED_PROGRAMS_GROUND_TASK_HPP_
#define TYR_PLANNING_LIFTED_PROGRAMS_GROUND_TASK_HPP_

#include "tyr/datalog/lifted/program.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/views.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/views.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/programs/translation_context.hpp"

#include <vector>
#include <yggdrasil/containers/associative_containers.hpp>
#include <yggdrasil/semantics/equal_to.hpp>
#include <yggdrasil/semantics/hash.hpp>

namespace tyr::planning
{

class GroundTaskProgram
{
public:
    using AppPredicateToActionMapping =
        ygg::UnorderedMap<formalism::datalog::PredicateView<formalism::FluentTag>, formalism::planning::ActionView<LiftedTag>>;
    using AppPredicateToAxiomMapping =
        ygg::UnorderedMap<formalism::datalog::PredicateView<formalism::FluentTag>, std::vector<formalism::planning::AxiomView<LiftedTag>>>;

    explicit GroundTaskProgram(formalism::planning::TaskView task);

    const TranslationContext<LiftedTag>& get_translation_context() const noexcept;
    const AppPredicateToActionMapping& get_predicate_to_action_mapping() const noexcept;
    const AppPredicateToAxiomMapping& get_predicate_to_axiom_mapping() const noexcept;
    datalog::Program<LiftedTag>& get_datalog_program() noexcept;
    const datalog::Program<LiftedTag>& get_datalog_program() const noexcept;
    const datalog::ConstProgramWorkspace<LiftedTag>& get_const_program_workspace() const noexcept;

private:
    TranslationContext<LiftedTag> m_translation_context;
    AppPredicateToActionMapping m_predicate_to_actions;
    AppPredicateToAxiomMapping m_predicate_to_axioms;

    datalog::Program<LiftedTag> m_datalog_program;
};

}

#endif
