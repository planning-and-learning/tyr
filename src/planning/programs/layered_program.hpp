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

#ifndef TYR_SRC_PLANNING_PROGRAMS_LAYERED_PROGRAM_HPP_
#define TYR_SRC_PLANNING_PROGRAMS_LAYERED_PROGRAM_HPP_

#include "common.hpp"
#include "tyr/datalog/rule_mapping.hpp"
#include "tyr/datalog/static_literal_decomposition.hpp"
#include "tyr/formalism/atom_view.hpp"
#include "tyr/formalism/binary_operator_view.hpp"
#include "tyr/formalism/conjunctive_condition_view.hpp"
#include "tyr/formalism/datalog/program_view.hpp"
#include "tyr/formalism/datalog/repository.hpp"
#include "tyr/formalism/datalog/rule_body_view.hpp"
#include "tyr/formalism/datalog/rule_view.hpp"
#include "tyr/formalism/declarations.hpp"
#include "tyr/formalism/function_term_value_view.hpp"
#include "tyr/formalism/function_term_view.hpp"
#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/literal_view.hpp"
#include "tyr/formalism/multi_operator_view.hpp"
#include "tyr/formalism/numeric_effect_view.hpp"
#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/parameter_index.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "tyr/formalism/unary_operator_view.hpp"
#include "tyr/formalism/variable_view.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <yggdrasil/formalism/row_view.hpp>

namespace tyr::planning
{

/// A Datalog program and the repository layer that holds it (over its task's repository).
struct LayeredProgram
{
    formalism::ProgramView<LiftedTag> program;
    formalism::RepositoryPtr repository;
};

/// Applies a program transformation, `pass(program, destination, rules) -> program`, in a layer over the source's: the
/// pass adds only its new records (the source's are found in the layers below, so the tables that refer to them stay
/// valid) and maps every source rule in `rules`.
template<typename Pass>
LayeredProgram transform(const LayeredProgram& source, std::size_t num_objects, datalog::RuleMapping& rules, Pass&& pass)
{
    auto repository = layer_over(source.repository, num_objects);
    const auto program = std::forward<Pass>(pass)(source.program, *repository, rules);
    return { program, std::move(repository) };
}

inline auto decompose_static_literals_pass()
{
    return [](formalism::ProgramView<LiftedTag> program, formalism::Repository& destination, datalog::RuleMapping& rules)
    {
        auto result = datalog::StaticLiteralDecomposition {};
        const auto output = datalog::decompose_static_literals(program, destination, result);
        rules = std::move(result.rules);
        return output;
    };
}

}

#endif
