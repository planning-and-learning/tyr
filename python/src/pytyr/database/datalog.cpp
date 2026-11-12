#include "module.hpp"
#include "pytyr/module.hpp"
#include "tyr/datalog/lifted/database/datalog/construction.hpp"
#include "tyr/datalog/lifted/database/datalog/evaluator.hpp"
#include "yggdrasil/python/bindings.hpp"

#include <nanobind/operators.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <optional>
#include <string>
#include <urd/python/sequences/sequence.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>
#include <utility>

namespace tyr
{
namespace
{
namespace dl = tyr::datalog::database::datalog;
using Repository = dl::DatalogRepository;
template<typename T>
using Sequence = urd::Store<urd::sequence<T>>;

template<typename T, typename V>
void set_indices(urd::flat_vector<urd::index<T>>& target, const Sequence<V>& views)
{
    target.clear();
    for (const auto& view : views)
        target.push_back(view.index());
}

template<typename T>
void set_values(urd::flat_vector<T>& target, const Sequence<T>& values)
{ target.set(values.begin(), values.end()); }

template<typename Input>
void bind_inputs(nb::class_<dl::Evaluator>& cls)
{
    using Inputs = Sequence<Input>;
    using Changes = Sequence<std::pair<Input, Input>>;
    cls.def("evaluate", &dl::Evaluator::template evaluate<Inputs>, nb::arg("inputs"))
        .def("initialize", &dl::Evaluator::template initialize<Inputs>, nb::arg("inputs"))
        .def("update", &dl::Evaluator::template update<Changes>, nb::arg("changes"), "One exact (added, removed) pair per input slot.");
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

    nb::enum_<dl::Comparison::Kind>(m, "DatalogComparisonKind")
        .value("EQUAL", dl::Comparison::Kind::EQUAL)
        .value("NOT_EQUAL", dl::Comparison::Kind::NOT_EQUAL)
        .value("LESS", dl::Comparison::Kind::LESS)
        .value("LESS_EQUAL", dl::Comparison::Kind::LESS_EQUAL);
    nb::class_<dl::Comparison>(m, "DatalogComparison")
        .def(
            "__init__",
            [](dl::Comparison* self, dl::Comparison::Kind kind, dl::Term lhs, dl::Term rhs) { new (self) dl::Comparison { kind, lhs, rhs }; },
            "kind"_a,
            "lhs"_a,
            "rhs"_a)
        .def_ro("kind", &dl::Comparison::kind)
        .def_ro("lhs", &dl::Comparison::lhs)
        .def_ro("rhs", &dl::Comparison::rhs);

    nb::enum_<dl::Arithmetic::Operation>(m, "DatalogArithmeticOperation")
        .value("ADD", dl::Arithmetic::Operation::ADD)
        .value("SUBTRACT", dl::Arithmetic::Operation::SUBTRACT)
        .value("MULTIPLY", dl::Arithmetic::Operation::MULTIPLY)
        .value("DIVIDE", dl::Arithmetic::Operation::DIVIDE)
        .value("MIN", dl::Arithmetic::Operation::MIN)
        .value("MAX", dl::Arithmetic::Operation::MAX);
    nb::class_<dl::Arithmetic>(m, "DatalogArithmetic", "result = lhs operation rhs over saturating numbers; the largest value is infinity.")
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

    auto repository = nb::class_<Repository>(m, "DatalogRepository", "Interns validated Datalog relations, atoms, aggregates, rules and programs.");
    repository.def(nb::init<>());

    {
        using V = urd::data<dl::Relation>;
        urd::python::bind_data<V>(m, "DatalogRelationData")
            .def(
                "__init__",
                [](V* self, size_t id, size_t arity, std::optional<size_t> input_slot, const std::string& name, bool weighted)
                { new (self) V(id, arity, input_slot.value_or(V::derived), name, weighted); },
                "id"_a,
                "arity"_a,
                "input_slot"_a = std::nullopt,
                "name"_a = "",
                "weighted"_a = false,
                "An input relation (with its input slot) or a derived relation; a weighted one keeps the least last column per row "
                "of the others.");
        using W = dl::RelationView;
        auto cls = nb::class_<W>(m, "DatalogRelation");
        cls.def("index", &W::index)
            .def("get_id", &W::get_id)
            .def("get_arity", &W::get_arity)
            .def("get_name", [](const W& self) { return std::string(self.get_name()); })
            .def("is_input", &W::is_input)
            .def("is_weighted", &W::is_weighted)
            .def("get_input_slot", &W::get_input_slot);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
        ygg::bind_insert<dl::Relation>(repository);
    }
    {
        using V = urd::data<dl::Atom>;
        urd::python::bind_data<V>(m, "DatalogAtomData")
            .def(
                "__init__",
                [](V* self, dl::RelationView relation, const Sequence<dl::Term>& terms)
                {
                    new (self) V();
                    self->relation = relation.index();
                    set_values(self->terms, terms);
                },
                "relation"_a,
                "terms"_a);
        using W = dl::AtomView;
        auto cls = nb::class_<W>(m, "DatalogAtom");
        cls.def("index", &W::index)
            .def("get_relation", &W::get_relation)
            .def("get_terms", [](const W& self) { return Sequence<dl::Term>(self.get_terms().begin(), self.get_terms().end()); });
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
        ygg::bind_insert<dl::Atom>(repository);
    }
    {
        using V = urd::data<dl::Aggregate>;
        urd::python::bind_data<V>(m, "DatalogAggregateData")
            .def(
                "__init__",
                [](V* self,
                   dl::AggregateFunction function,
                   ygg::uint_t result,
                   ygg::uint_t value,
                   const Sequence<dl::AtomView>& atoms,
                   const Sequence<dl::AtomView>& negated,
                   const Sequence<dl::Comparison>& comparisons,
                   const Sequence<dl::Arithmetic>& arithmetic)
                {
                    new (self) V();
                    self->function = function;
                    self->result = result;
                    self->value = value;
                    set_indices(self->atoms, atoms);
                    set_indices(self->negated, negated);
                    set_values(self->comparisons, comparisons);
                    set_values(self->arithmetic, arithmetic);
                },
                "function"_a,
                "result"_a,
                "value"_a,
                "atoms"_a,
                "negated"_a = Sequence<dl::AtomView> {},
                "comparisons"_a = Sequence<dl::Comparison> {},
                "arithmetic"_a = Sequence<dl::Arithmetic> {},
                "result = function value : atoms, arithmetic, not negated, comparisons; COUNT ignores value.");
        using W = dl::AggregateView;
        auto cls = nb::class_<W>(m, "DatalogAggregate");
        cls.def("index", &W::index).def("get_function", &W::get_function).def("get_result", &W::get_result).def("get_value", &W::get_value);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
        ygg::bind_insert<dl::Aggregate>(repository);
    }
    {
        using V = urd::data<dl::Rule>;
        urd::python::bind_data<V>(m, "DatalogRuleData")
            .def(
                "__init__",
                [](V* self,
                   dl::AtomView head,
                   const Sequence<dl::AtomView>& atoms,
                   const Sequence<dl::AtomView>& negated,
                   const Sequence<dl::Comparison>& comparisons,
                   const Sequence<dl::AggregateView>& aggregates,
                   const Sequence<dl::Arithmetic>& arithmetic)
                {
                    new (self) V();
                    self->head = head.index();
                    set_indices(self->atoms, atoms);
                    set_indices(self->negated, negated);
                    set_values(self->comparisons, comparisons);
                    set_indices(self->aggregates, aggregates);
                    set_values(self->arithmetic, arithmetic);
                },
                "head"_a,
                "atoms"_a,
                "negated"_a = Sequence<dl::AtomView> {},
                "comparisons"_a = Sequence<dl::Comparison> {},
                "aggregates"_a = Sequence<dl::AggregateView> {},
                "arithmetic"_a = Sequence<dl::Arithmetic> {});
        using W = dl::RuleView;
        auto cls = nb::class_<W>(m, "DatalogRule");
        cls.def("index", &W::index).def("get_head", &W::get_head);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
        ygg::bind_insert<dl::Rule>(repository);
    }
    {
        using V = urd::data<dl::Program>;
        urd::python::bind_data<V>(m, "DatalogProgramData")
            .def(
                "__init__",
                [](V* self, const Sequence<dl::RuleView>& rules)
                {
                    new (self) V();
                    set_indices(self->rules, rules);
                },
                "rules"_a);
        using W = dl::ProgramView;
        auto cls = nb::class_<W>(m, "DatalogProgram");
        cls.def("index", &W::index);
        urd::python::add_comparison(cls);
        urd::python::add_hash(cls);
        ygg::bind_insert<dl::Program>(repository);
    }

    {
        using E = dl::Evaluator;
        auto cls = nb::class_<E>(m,
                                 "DatalogEvaluator",
                                 "Evaluate a program's relations rule by rule (semi-naive), and maintain them under input changes (counting, "
                                 "Delete and Rederive). Its results are read through the relation repository. Static input slots are read once.");
        cls.def(
               "__init__",
               [](E* self,
                  const ygg::FreeRelationRepository& relations,
                  dl::ProgramView program,
                  const Sequence<dl::RelationView>& outputs,
                  const Sequence<size_t>& static_slots) { new (self) E(relations, program, outputs, static_slots); },
               "repository"_a,
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
        bind_inputs<ygg::FreeBuilderRelationView>(cls);
        bind_inputs<ygg::FreeRelationView>(cls);
    }
}
}  // namespace tyr
