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

#ifndef TYR_DATALOG_POLICIES_TERMINATION_CONCEPTS_HPP_
#define TYR_DATALOG_POLICIES_TERMINATION_CONCEPTS_HPP_

#include "tyr/datalog/fact_sets.hpp"
#include "tyr/datalog/policies/annotation_types.hpp"
#include "tyr/declarations.hpp"
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

#include <concepts>

namespace tyr::datalog
{

template<typename T>
concept TerminationPolicyConcept = requires(T& p,
                                            const T& cp,
                                            formalism::datalog::ConjunctiveConditionView<GroundTag> goals,
                                            const FactSets& fact_sets,
                                            const PredicateAnnotations<>& annotations,
                                            const FunctionAnnotations<>& numeric_annotations,
                                            const NumericSupportSelector& numeric_support_selector) {
    { p.set_goals(goals) } -> std::same_as<void>;
    { cp.check(fact_sets) } -> std::same_as<bool>;
    { cp.should_terminate(fact_sets) } -> std::same_as<bool>;
    { cp.get_total_cost(fact_sets, annotations, numeric_annotations, numeric_support_selector) } -> std::same_as<Cost>;
    { p.reset() } -> std::same_as<void>;
    { p.clear() } -> std::same_as<void>;
};

}

#endif
