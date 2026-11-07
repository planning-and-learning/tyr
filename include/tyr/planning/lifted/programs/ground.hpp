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
#include "tyr/formalism/datalog/atom_view.hpp"
#include "tyr/formalism/datalog/binary_operator_view.hpp"
#include "tyr/formalism/datalog/conditional_effect_view.hpp"
#include "tyr/formalism/datalog/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/conjunctive_effect_view.hpp"
#include "tyr/formalism/datalog/function_term_value_view.hpp"
#include "tyr/formalism/datalog/function_term_view.hpp"
#include "tyr/formalism/datalog/literal_view.hpp"
#include "tyr/formalism/datalog/multi_operator_view.hpp"
#include "tyr/formalism/datalog/numeric_effect_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/datalog/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/planning/atom_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/planning/binary_operator_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/planning/function_term_value_view.hpp"
#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/planning/literal_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/planning/multi_operator_view.hpp"
#include "tyr/formalism/planning/numeric_effect_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/planning/unary_operator_view.hpp"
#include "tyr/planning/declarations.hpp"
#include "tyr/planning/lifted/programs/translation_context.hpp"

#include <urd/elements/equal_to.hpp>
#include <urd/elements/hash.hpp>
#include <urd/repr/maps/std_map.hpp>
#include <urd/repr/maps/unordered_flat_map.hpp>
#include <urd/repr/values/unordered_flat_set.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>
#include <vector>

namespace tyr::planning
{

class GroundTaskProgram
{
public:
    using AppPredicateToAxiomMapping =
        urd::Store<urd::map<formalism::datalog::PredicateView<formalism::FluentTag>, urd::Store<urd::sequence<formalism::planning::AxiomView<LiftedTag>>>>>;

    explicit GroundTaskProgram(formalism::planning::TaskView<LiftedTag> task);

    [[nodiscard]] const TranslationContext<LiftedTag>& get_translation_context() const noexcept;
    [[nodiscard]] const AppPredicateToActionMapping& get_predicate_to_action_mapping() const noexcept;
    [[nodiscard]] const AppPredicateToAxiomMapping& get_predicate_to_axiom_mapping() const noexcept;
    [[nodiscard]] const ConditionalEffectToPredicateMapping& get_conditional_effect_to_predicate_mapping() const noexcept;
    datalog::Program<LiftedTag>& get_datalog_program() noexcept;
    [[nodiscard]] const datalog::Program<LiftedTag>& get_datalog_program() const noexcept;
    [[nodiscard]] const datalog::ConstProgramWorkspace<LiftedTag>& get_const_program_workspace() const noexcept;

private:
    TranslationContext<LiftedTag> m_translation_context;
    AppPredicateToActionMapping m_predicate_to_actions;
    AppPredicateToAxiomMapping m_predicate_to_axioms;
    ConditionalEffectToPredicateMapping m_conditional_effect_to_predicate;

    datalog::Program<LiftedTag> m_datalog_program;
};

}

#endif
