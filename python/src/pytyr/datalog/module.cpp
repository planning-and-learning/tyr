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

#include "module.hpp"

#include "datalog.hpp"
#include "ground/module.hpp"
#include "lifted/module.hpp"

namespace tyr::datalog
{

namespace
{

void alias_common_api(nb::module_& source, nb::module_& target)
{
    static constexpr const char* names[] = {
        "NumericSupport",
        "BaseAnnotation",
        "WitnessAnnotation",
        "FunctionWitnessAnnotation",
        "PredicateAnnotations",
        "FunctionAnnotations",
        "NoAnnotationPolicy",
        "SumMinCostAnnotationPolicy",
        "MaxMinCostAnnotationPolicy",
        "MaxMinCostAnnotationWithAchieversPolicy",
        "NoTerminationPolicy",
        "SumTerminationPolicy",
        "MaxTerminationPolicy",
        "RuleCostPolicy",
    };
    for (const auto* name : names)
        target.attr(name) = source.attr(name);
}

}

void bind_module_definitions(nb::module_& m)
{
    bind_fact_sets(m);
    bind_numeric_support(m);
    bind_annotations(m);
    bind_policies(m);

    nb::class_<ProgramStatistics>(m, "ProgramStatistics")
        .def(nb::init<>())
        .def_rw("num_executions", &ProgramStatistics::num_executions)
        .def_rw("parallel_time", &ProgramStatistics::parallel_time)
        .def_rw("total_time", &ProgramStatistics::total_time);

    nb::class_<GroundQueueStatistics>(m, "GroundQueueStatistics")
        .def(nb::init<>())
        .def_rw("num_queue_pushes", &GroundQueueStatistics::num_queue_pushes)
        .def_rw("num_queue_pops", &GroundQueueStatistics::num_queue_pops)
        .def_rw("num_stale_queue_pops", &GroundQueueStatistics::num_stale_queue_pops)
        .def_rw("num_rules_fired", &GroundQueueStatistics::num_rules_fired)
        .def_rw("num_facts_derived", &GroundQueueStatistics::num_facts_derived)
        .def_rw("max_queue_size", &GroundQueueStatistics::max_queue_size);

    auto ground_module = m.def_submodule("ground");
    alias_common_api(m, ground_module);
    bind_ground_module_definitions(ground_module);

    auto lifted_module = m.def_submodule("lifted");
    alias_common_api(m, lifted_module);
    bind_lifted_module_definitions(lifted_module);
}

}
