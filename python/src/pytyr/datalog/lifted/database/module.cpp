#include "module.hpp"

#include "pytyr/module.hpp"
#include "tyr/datalog/lifted/database/semantics/operations.hpp"
#include "tyr/datalog/lifted/database/semantics/plans.hpp"
#include "yggdrasil/formalism/scratch_relation.hpp"

#include <initializer_list>
#include <nanobind/stl/vector.h>
#include <span>
#include <vector>

namespace tyr
{

namespace
{
using Column = urd::index<ygg::Column>;
using Relation = ygg::FreeScratchRelation;
using ColumnIndices = std::span<const urd::data<ygg::ColumnLayout>>;
using Workspace = tyr::datalog::database::Workspace;

/// The plans, built from their inputs' columns (validated there), and the scratch storage of the operations.
void bind_plans(nb::module_& m)
{
    using namespace tyr::datalog::database;
    nb::class_<Workspace>(m, "Workspace", "Scratch storage of the relational operations; reuse one, never concurrently.")
        .def(nb::init<>())
        .def("memory_usage", &Workspace::memory_usage);
    nb::class_<ProjectionPlan>(m, "ProjectionPlan", "Keeps the input's columns of these labels, in this order.")
        .def(nb::init<ColumnIndices, urd::Store<urd::sequence<Column>>>(), nb::arg("input"), nb::arg("labels"))
        .def("input_columns", &ProjectionPlan::input_columns)
        .def("output_columns", &ProjectionPlan::output_columns);
    nb::class_<JoinPlan>(m, "JoinPlan", "Natural join (also semijoin, antijoin) of inputs with these columns.")
        .def(nb::init<ColumnIndices, ColumnIndices>(), nb::arg("left"), nb::arg("right"))
        .def("left_columns", &JoinPlan::left_columns)
        .def("right_columns", &JoinPlan::right_columns)
        .def("output_columns", &JoinPlan::output_columns);
    nb::class_<SetPlan>(m, "SetPlan", "Union and difference of inputs with the same columns in the same order.")
        .def(nb::init<ColumnIndices, ColumnIndices>(), nb::arg("left"), nb::arg("right"))
        .def("output_columns", &SetPlan::output_columns);
    nb::class_<SelectEqualPlan>(m, "SelectEqualPlan", "The rows whose two columns (of one type) hold equal values.")
        .def(nb::init<ColumnIndices, Column, Column>(), nb::arg("input"), nb::arg("left"), nb::arg("right"))
        .def("output_columns", &SelectEqualPlan::output_columns);
    nb::class_<SelectValuePlan>(m, "SelectValuePlan", "The rows whose column holds a value of the column's type.")
        .def(
            "__init__",
            [](SelectValuePlan* self, ColumnIndices input, Column column, ygg::TypeId type, nb::handle value)
            { new (self) SelectValuePlan(input, column, type, ygg::encode(ygg::cast_value(value, type))); },
            nb::arg("input"),
            nb::arg("column"),
            nb::arg("type"),
            nb::arg("value"))
        .def("output_columns", &SelectValuePlan::output_columns);
}

/// The operations of one input relation type.
template<typename V>
void bind_unary(nb::module_& m)
{
    using namespace tyr::datalog::database;
    m.def(
        "project",
        [](const V& input, const ProjectionPlan& plan, Relation& out, Workspace& workspace)
        {
            require_columns(input.columns(), plan.input_columns());
            require_columns(out.view().columns(), plan.output_columns());
            project(input, plan, out, workspace);
        },
        nb::arg("input"),
        nb::arg("plan"),
        nb::arg("out"),
        nb::arg("workspace"));
    m.def(
        "select_equal_columns",
        [](const V& input, const SelectEqualPlan& plan, Relation& out)
        {
            require_columns(input.columns(), plan.output_columns());
            require_columns(out.view().columns(), plan.output_columns());
            select_equal_columns(input, plan, out);
        },
        nb::arg("input"),
        nb::arg("plan"),
        nb::arg("out"));
    m.def(
        "select_equal_value",
        [](const V& input, const SelectValuePlan& plan, Relation& out)
        {
            require_columns(input.columns(), plan.output_columns());
            require_columns(out.view().columns(), plan.output_columns());
            select_equal_value(input, plan, out);
        },
        nb::arg("input"),
        nb::arg("plan"),
        nb::arg("out"));
    m.def(
        "assign",
        [](nb::typed<nb::handle, Relation> destination, const V& source)
        {
            assign(nb::cast<Relation&>(destination), source);
            return destination;
        },
        nb::arg("destination"),
        nb::arg("source"));
}

/// The operations of one pair of input relation types.
template<typename L, typename R>
void bind_binary(nb::module_& m)
{
    using namespace tyr::datalog::database;
    const auto bind_join = [&m](const char* name, auto operation, bool output_is_left)
    {
        m.def(
            name,
            [operation, output_is_left](const L& left_operand, const R& right_operand, const JoinPlan& plan, Relation& out, Workspace& workspace)
            {
                require_columns(left_operand.columns(), plan.left_columns());
                require_columns(right_operand.columns(), plan.right_columns());
                require_columns(out.view().columns(), output_is_left ? plan.left_columns() : plan.output_columns());
                operation(left_operand, right_operand, plan, out, workspace);
            },
            nb::arg("left_operand"),
            nb::arg("right_operand"),
            nb::arg("plan"),
            nb::arg("out"),
            nb::arg("workspace"));
    };
    bind_join("join", [](auto&&... args) { join(args...); }, false);
    bind_join("semijoin", [](auto&&... args) { semijoin(args...); }, true);
    bind_join("antijoin", [](auto&&... args) { antijoin(args...); }, true);
    const auto bind_set = [&m](const char* name, auto operation)
    {
        m.def(
            name,
            [operation](const L& left_operand, const R& right_operand, const SetPlan& plan, Relation& out)
            {
                for (const auto columns : { left_operand.columns(), right_operand.columns(), out.view().columns() })
                    require_columns(columns, plan.output_columns());
                operation(left_operand, right_operand, plan, out);
            },
            nb::arg("left_operand"),
            nb::arg("right_operand"),
            nb::arg("plan"),
            nb::arg("out"));
    };
    bind_set("union", [](auto&&... args) { union_(args...); });
    bind_set("difference", [](auto&&... args) { difference(args...); });
}

/// The relations the operations write and the evaluators draw: each a builder with a repository of its own rows.
void bind_scratch_relations(nb::module_& m)
{
    nb::class_<Relation>(m, "ScratchRelation", "A relation being built, its rows in tuple order in a repository of its own: an operation's output.")
        .def(
            "__init__",
            [](Relation* self)
            {
                new (self) Relation();
                self->relation.initialize(self->rows, self->rows.schema(std::span<const urd::data<ygg::ColumnLayout>>()), {});
            },
            "The nullary relation, empty.")
        .def(
            "__init__",
            [](Relation* self, ColumnIndices columns)
            {
                new (self) Relation();
                self->relation.initialize(self->rows, self->rows.schema(columns), columns);
            },
            nb::arg("columns"))
        .def(
            "initialize",
            [](Relation& relation, ColumnIndices columns) { relation.relation.initialize(relation.rows, relation.rows.schema(columns), columns); },
            nb::arg("columns"))
        .def(
            "rename",
            [](Relation& relation, ColumnIndices columns) { relation.relation.rename(relation.rows, columns); },
            nb::arg("columns"))
        .def(
            "insert",
            [](Relation& relation, std::span<const ygg::Value> values) { return relation.insert(values).first; },
            nb::arg("values"),
            "Its position in tuple order.")
        .def(
            "erase",
            [](Relation& relation, std::span<const ygg::Value> values) { return relation.erase(values); },
            nb::arg("values"))
        .def("clear", [](Relation& relation) { relation.clear(); })
        .def("columns",
             [](const Relation& relation)
             { return std::vector<urd::data<ygg::ColumnLayout>>(relation.view().columns().begin(), relation.view().columns().end()); })
        .def("size", [](const Relation& relation) { return relation.view().get_rows().size(); })
        .def("arity", [](const Relation& relation) { return relation.view().columns().size(); })
        .def("empty", [](const Relation& relation) { return relation.view().get_rows().size() == 0; })
        .def("__len__", [](const Relation& relation) { return relation.view().get_rows().size(); })
        .def(
            "contains",
            [](const Relation& relation, std::span<const ygg::Value> values) { return relation.view().contains(values); },
            nb::arg("values"))
        .def("view", &Relation::view, nb::keep_alive<0, 1>(), "Its builder, read through its repository; the next change invalidates the view.");
    nb::class_<ygg::FreeScratchRelations>(m, "ScratchRelations", "The scratch relations an evaluator draws; they must outlive it.").def(nb::init<>());
}
}  // namespace

void bind_database_module_definitions(nb::module_& m)
{
    using RelationView = ygg::FreeRelationView;
    using RelationBuilderView = ygg::FreeRelationBuilderView;
    bind_scratch_relations(m);
    bind_plans(m);
    bind_unary<RelationView>(m);
    bind_unary<RelationBuilderView>(m);
    bind_binary<RelationView, RelationView>(m);
    bind_binary<RelationView, RelationBuilderView>(m);
    bind_binary<RelationBuilderView, RelationView>(m);
    bind_binary<RelationBuilderView, RelationBuilderView>(m);
    bind_database_queries(m);
    bind_database_optimization(m);
    bind_database_evaluation(m);
    bind_database_distance(m);
    bind_database_datalog(m);
}

}  // namespace tyr
