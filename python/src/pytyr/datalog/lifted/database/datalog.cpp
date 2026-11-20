#include "module.hpp"
#include "pytyr/module.hpp"
#include "tyr/datalog/lifted/database/datalog/construction.hpp"
#include "tyr/datalog/lifted/database/datalog/evaluator.hpp"
#include "yggdrasil/python/bindings.hpp"

#include <nanobind/operators.h>
#include <nanobind/stl/pair.h>
#include <urd/python/sequences/sequence.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>

namespace tyr
{
namespace
{
namespace dl = tyr::datalog::database::datalog;
template<typename T>
using Sequence = urd::Store<urd::sequence<T>>;

template<typename Input>
void bind_inputs(nb::class_<dl::Evaluator>& cls)
{
    using Inputs = Sequence<Input>;
    using Changes = Sequence<std::pair<Input, Input>>;
    cls.def("evaluate", &dl::Evaluator::template evaluate<Inputs>, nb::arg("inputs"))
        .def("initialize", &dl::Evaluator::template initialize<Inputs>, nb::arg("inputs"))
        .def("update", &dl::Evaluator::template update<Changes>, nb::arg("changes"), "One exact (added, removed) pair per input slot.");
}

/// A Datalog record: its index, its record (Python builds it through its empty __init__ and its fields), its builder,
/// its view and DatalogRepository.insert of the record, prepared first (prepare_for_insert).
template<typename T>
void bind_record(nb::module_& m, nb::class_<dl::DatalogRepository>& repository)
{
    urd::python::bind_index<T>(m);
    urd::python::bind_data<T>(m);
    urd::python::bind_builder<T, dl::DatalogRepository>(m);
    urd::python::bind_view<urd::index_view<T, dl::DatalogRepository>>(m);
    ygg::bind_insert<T>(repository);
}
}  // namespace

void bind_database_datalog(nb::module_& m)
{
    using namespace nb::literals;

    nb::enum_<dl::AggregateFunction>(m, "DatalogAggregateFunction")
        .value("COUNT", dl::AggregateFunction::COUNT)
        .value("SUM", dl::AggregateFunction::SUM)
        .value("MIN", dl::AggregateFunction::MIN)
        .value("MAX", dl::AggregateFunction::MAX);

    auto term = nb::class_<dl::Term>(m, "DatalogTerm", "A variable, numbered within its rule, or an object constant.");
    term.def_static("variable", &dl::Term::variable, "variable"_a)
        .def_static("constant", &dl::Term::constant, "object"_a)
        .def("is_variable", &dl::Term::is_variable)
        .def_ro("value", &dl::Term::value)
        .def(nb::self == nb::self);

    auto comparison = nb::class_<dl::Comparison>(m, "DatalogComparison", "lhs kind rhs, ordered as uint_t (the largest value is infinity).");
    nb::enum_<dl::Comparison::Kind>(m, "DatalogComparisonKind")
        .value("EQUAL", dl::Comparison::Kind::EQUAL)
        .value("NOT_EQUAL", dl::Comparison::Kind::NOT_EQUAL)
        .value("LESS", dl::Comparison::Kind::LESS)
        .value("LESS_EQUAL", dl::Comparison::Kind::LESS_EQUAL);
    comparison
        .def(
            "__init__",
            [](dl::Comparison* self, dl::Comparison::Kind kind, dl::Term lhs, dl::Term rhs) { new (self) dl::Comparison { kind, lhs, rhs }; },
            "kind"_a,
            "lhs"_a,
            "rhs"_a)
        .def_ro("kind", &dl::Comparison::kind)
        .def_ro("lhs", &dl::Comparison::lhs)
        .def_ro("rhs", &dl::Comparison::rhs);

    auto arithmetic = nb::class_<dl::Arithmetic>(m, "DatalogArithmetic", "result = lhs operation rhs over saturating numbers; the largest value is infinity.");
    nb::enum_<dl::Arithmetic::Operation>(m, "DatalogArithmeticOperation")
        .value("ADD", dl::Arithmetic::Operation::ADD)
        .value("SUBTRACT", dl::Arithmetic::Operation::SUBTRACT)
        .value("MULTIPLY", dl::Arithmetic::Operation::MULTIPLY)
        .value("DIVIDE", dl::Arithmetic::Operation::DIVIDE)
        .value("MIN", dl::Arithmetic::Operation::MIN)
        .value("MAX", dl::Arithmetic::Operation::MAX);
    arithmetic
        .def(
            "__init__",
            [](dl::Arithmetic* self, dl::Arithmetic::Operation operation, dl::Term lhs, dl::Term rhs, ygg::uint_t result)
            { new (self) dl::Arithmetic { operation, lhs, rhs, result }; },
            "operation"_a,
            "lhs"_a,
            "rhs"_a,
            "result"_a)
        .def_ro("operation", &dl::Arithmetic::operation)
        .def_ro("lhs", &dl::Arithmetic::lhs)
        .def_ro("rhs", &dl::Arithmetic::rhs)
        .def_ro("result", &dl::Arithmetic::result);

    auto repository = nb::class_<dl::DatalogRepository>(m, "DatalogRepository", "Interns validated Datalog relations, atoms, aggregates, rules and programs.");
    repository.def(nb::init<>());
    bind_record<dl::Relation>(m, repository);
    bind_record<dl::Atom>(m, repository);
    bind_record<dl::Aggregate>(m, repository);
    bind_record<dl::Rule>(m, repository);
    bind_record<dl::Program>(m, repository);

    {
        using E = dl::Evaluator;
        auto cls = nb::class_<E>(m,
                                 "DatalogEvaluator",
                                 "Evaluate a program's relations rule by rule (semi-naive), and maintain them under input changes (counting, "
                                 "Delete and Rederive). Its results are drawn from the scratch relations. Static input slots are read once.");
        cls.def(
               "__init__",
               [](E* self,
                  ygg::FreeScratchRelations& scratch,
                  dl::ProgramView program,
                  const Sequence<dl::RelationView>& outputs,
                  const Sequence<size_t>& static_slots) { new (self) E(scratch, program, outputs, static_slots); },
               "scratch"_a,
               "program"_a,
               "outputs"_a,
               "static_slots"_a = Sequence<size_t> {},
               nb::keep_alive<1, 2>(),
               nb::keep_alive<1, 3>())
            .def(
                "get_result",
                [](const E& evaluator, size_t output) { return evaluator.get_result(output); },
                "output"_a = 0,
                nb::keep_alive<0, 1>(),
                "Borrowed result; the next evaluate, initialize or update invalidates it.")
            .def(
                "get_delta",
                [](const E& evaluator, size_t output) -> const tyr::datalog::database::incremental::Delta& { return evaluator.get_delta(output); },
                "output"_a = 0,
                nb::rv_policy::reference_internal)
            .def("memory_usage", &E::memory_usage);
        bind_inputs<ygg::FreeRelationBuilderView>(cls);
        bind_inputs<ygg::FreeRelationView>(cls);
    }
}
}  // namespace tyr
