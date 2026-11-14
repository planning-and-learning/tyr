#include "tyr/datalog/lifted/database/semantics/distance.hpp"

#include "module.hpp"
#include "pytyr/module.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/distance.hpp"

#include <nanobind/stl/pair.h>
#include <span>
#include <stdexcept>
#include <utility>

namespace tyr
{

namespace
{
using Relation = urd::builder<ygg::Relation<ygg::Schema>>;
using Column = urd::index<ygg::Column>;
using ColumnIndices = std::span<const ygg::ColumnLayout>;
using Plan = tyr::datalog::database::DistancePlan;
using Evaluator = tyr::datalog::database::incremental::DistanceEvaluator;
using Delta = tyr::datalog::database::incremental::Delta;
using Workspace = tyr::datalog::database::DistanceWorkspace;

/// Binds distance and the evaluator members for one input relation type.
template<typename Input>
void bind_inputs(nb::class_<Evaluator>& evaluator, nb::module_& m)
{
    using Change = std::pair<Input, Input>;
    m.def(
        "distance",
        [](const Input& sources, const Input& edges, const Input& targets, const Plan& plan, Relation& out, Workspace& workspace)
        {
            require_columns(sources.columns(), plan.source_columns());
            require_columns(edges.columns(), plan.edge_columns());
            require_columns(targets.columns(), plan.target_columns());
            require_columns(out.columns(), plan.output_columns());
            if (!workspace.matches(plan))
                throw std::invalid_argument("Distance: workspace vertex size does not match the plan.");
            tyr::datalog::database::distance(sources, edges, targets, plan, out, workspace);
        },
        nb::arg("sources"),
        nb::arg("edges"),
        nb::arg("targets"),
        nb::arg("plan"),
        nb::arg("out"),
        nb::arg("workspace"),
        "Replaces out's rows by the reachable source/target tuples and their shortest distance, including zero-length paths.");
    evaluator.def("initialize", &Evaluator::template initialize<Input, Input, Input>, nb::arg("sources"), nb::arg("edges"), nb::arg("targets"))
        .def("update",
             &Evaluator::template update<Change, Change, Change>,
             nb::arg("sources"),
             nb::arg("edges"),
             nb::arg("targets"),
             "Each argument is an (added, removed) pair of actual set changes.");
}
}  // namespace

void bind_database_distance(nb::module_& m)
{
    using Borrowed = ygg::FreeBuilderRelationView;

    nb::class_<Plan>(m, "DistancePlan", "Validated schemas for directed, unweighted distances between endpoint tuples.")
        .def(nb::init<ColumnIndices, ColumnIndices, ColumnIndices, Column>(),
             nb::arg("source_columns"),
             nb::arg("edge_columns"),
             nb::arg("target_columns"),
             nb::arg("distance_column"))
        .def("arity", &Plan::arity)
        .def("output_columns", [](const Plan& plan) { return plan.output_columns(); }, nb::keep_alive<0, 1>());

    nb::class_<Workspace>(m, "DistanceWorkspace", "Scratch storage of distance for one plan's vertex size; reuse one, never concurrently.")
        .def(nb::init<const Plan&>(), nb::arg("plan"))
        .def("memory_usage", &Workspace::memory_usage);

    nb::class_<Delta>(m, "RelationDelta", "Read-only added and removed rows from the last evaluator update. The next update replaces them.")
        .def_prop_ro(
            "added",
            [](const Delta& delta) { return delta.change().first; },
            nb::keep_alive<0, 1>())
        .def_prop_ro("removed", [](const Delta& delta) { return delta.change().second; }, nb::keep_alive<0, 1>());

    auto evaluator =
        nb::class_<Evaluator>(m, "DistanceEvaluator", "Maintains distances and exact output deltas. Input relations are borrowed only during each call.");
    evaluator.def(nb::init<const ygg::FreeRelationRepository&, Plan>(), nb::arg("repository"), nb::arg("plan"), nb::keep_alive<1, 2>())
        .def(
            "get_result",
            [](const Evaluator& self) { return self.get_result(); },
            nb::keep_alive<0, 1>(),
            "Borrowed result; the next initialize or update invalidates it.")
        .def(
            "get_delta",
            [](const Evaluator& self) -> const Delta& { return self.get_delta(); },
            nb::rv_policy::reference_internal)
        .def("memory_usage", &Evaluator::memory_usage);

    bind_inputs<Borrowed>(evaluator, m);
    bind_inputs<ygg::FreeRelationView>(evaluator, m);
}
}  // namespace tyr
