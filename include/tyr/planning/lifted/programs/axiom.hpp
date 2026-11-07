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

#ifndef TYR_PLANNING_LIFTED_PROGRAMS_AXIOM_HPP_
#define TYR_PLANNING_LIFTED_PROGRAMS_AXIOM_HPP_

#include "tyr/datalog/lifted/program.hpp"
#include "tyr/formalism/planning/action_view.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/planning/axiom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/planning/conditional_effect_view.hpp"
#include "tyr/formalism/planning/conjunctive_condition_view.hpp"
#include "tyr/formalism/planning/conjunctive_effect_view.hpp"
#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/fdr_fact_view.hpp"
#include "tyr/formalism/planning/fdr_variable_view.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/planning/invariant_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/planning/metric_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/planning/task_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include <yggdrasil/formalism/binding_view.hpp>
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/planning/lifted/programs/translation_context.hpp"

namespace tyr::planning
{

class AxiomEvaluatorProgram
{
public:
    explicit AxiomEvaluatorProgram(formalism::planning::TaskView<LiftedTag> task);

    [[nodiscard]] const TranslationContext<LiftedTag>& get_translation_context() const noexcept;
    datalog::Program<LiftedTag>& get_datalog_program() noexcept;
    [[nodiscard]] const datalog::Program<LiftedTag>& get_datalog_program() const noexcept;
    [[nodiscard]] const datalog::ConstProgramWorkspace<LiftedTag>& get_const_program_workspace() const noexcept;

private:
    TranslationContext<LiftedTag> m_translation_context;
    datalog::Program<LiftedTag> m_datalog_program;
};

}

#endif
