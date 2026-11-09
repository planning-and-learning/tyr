#include "module.hpp"
#include "pytyr/module.hpp"
#include <urd/python/sequences/sequence.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include "tyr/datalog/lifted/database/optimization/optimization.hpp"
#include "tyr/datalog/lifted/database/semantics/incremental/query.hpp"
#include "tyr/datalog/lifted/database/semantics/query_evaluation.hpp"

#include <nanobind/stl/pair.h>
#include <nanobind/stl/vector.h>
#include <utility>
#include <vector>

namespace tyr
{

namespace
{
using FullEvaluator = tyr::datalog::database::QueryEvaluator;
using IncrementalEvaluator = tyr::datalog::database::incremental::QueryEvaluator;

/// Binds the evaluator members for one input relation type.
template<typename Input>
void bind_inputs(nb::class_<FullEvaluator>& full, nb::class_<IncrementalEvaluator>& incremental, nb::module_& m)
{
    using Inputs = urd::Store<urd::sequence<Input>>;
    using Changes = urd::Store<urd::sequence<std::pair<Input, Input>>>;
    full.def("evaluate", &FullEvaluator::template evaluate<Inputs>, nb::arg("inputs"));
    incremental.def("initialize", &IncrementalEvaluator::template initialize<Inputs>, nb::arg("inputs"))
        .def("update", &IncrementalEvaluator::template update<Changes>, nb::arg("changes"), "One (added, removed) pair per input slot.");
    m.def("collect_statistics", &tyr::datalog::database::collect_statistics<Inputs>, nb::arg("inputs"));
}
}  // namespace

void bind_database_evaluation(nb::module_& m)
{
    using Repository = ygg::FreeRelationRepository;
    using QueryPlan = tyr::datalog::database::QueryPlan;

    auto full = nb::class_<FullEvaluator>(m,
                                          "QueryEvaluator",
                                          "Evaluate a compiled query DAG; its results are read through the repository. Input relations are "
                                          "borrowed only during each call.");
    full.def(nb::init<const Repository&, QueryPlan>(), nb::arg("repository"), nb::arg("plan"), nb::keep_alive<1, 2>())
        .def(
            "get_result",
            [](const FullEvaluator& evaluator, size_t root) { return evaluator.get_result(root); },
            nb::arg("root") = 0,
            nb::keep_alive<0, 1>(),
            "Borrowed result; the next evaluate invalidates it.")
        .def("memory_usage", &FullEvaluator::memory_usage);

    auto incremental = nb::class_<IncrementalEvaluator>(m, "IncrementalQueryEvaluator", "Maintain a compiled query DAG and exact net root deltas.");
    incremental.def(nb::init<const Repository&, QueryPlan>(), nb::arg("repository"), nb::arg("plan"), nb::keep_alive<1, 2>())
        .def(
            "get_result",
            [](const IncrementalEvaluator& evaluator, size_t root) { return evaluator.get_result(root); },
            nb::arg("root") = 0,
            nb::keep_alive<0, 1>(),
            "Borrowed result; the next initialize or update invalidates it.")
        .def(
            "get_delta",
            [](const IncrementalEvaluator& evaluator, size_t root) -> const tyr::datalog::database::incremental::Delta& { return evaluator.get_delta(root); },
            nb::arg("root") = 0,
            nb::rv_policy::reference_internal)
        .def("memory_usage", &IncrementalEvaluator::memory_usage);

    bind_inputs<ygg::FreeBuilderRelationView>(full, incremental, m);
    bind_inputs<ygg::FreeRelationView>(full, incremental, m);
}
}  // namespace tyr
