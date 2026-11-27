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

#ifndef TYR_PYTHON_PLANNING_AXIOMS_AXIOM_EVALUATOR_HPP_
#define TYR_PYTHON_PLANNING_AXIOMS_AXIOM_EVALUATOR_HPP_

#include "pytyr/planning/module.hpp"
#include <memory>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <string>
#include <tyr/planning/axioms/axiom_evaluator.hpp>
#include <tyr/planning/declarations.hpp>
#include <tyr/planning/tasks/task.hpp>
#include <urd/python/bindings.hpp>
#include <utility>
#include <yggdrasil/execution/onetbb.hpp>

namespace tyr::planning
{

template<TaskKind Kind>
void bind_axiom_evaluator(nb::module_& m, const std::string& name)
{
    using namespace nb::literals;
    using T = AxiomEvaluator<Kind>;

    nb::class_<T>(m, name.c_str())
        .def(nb::new_([](TaskPtr<Kind> task, ygg::ExecutionContextPtr execution_context)
                      { return std::make_shared<T>(std::move(task), std::move(execution_context)); }),
             "task"_a,
             "execution_context"_a);
}

}  // namespace tyr::planning

#endif
