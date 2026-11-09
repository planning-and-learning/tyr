#include "module.hpp"
#include "pytyr/module.hpp"

#include "tyr/datalog/lifted/database/semantics/operations.hpp"
#include "tyr/datalog/lifted/database/semantics/plans.hpp"

#include <initializer_list>
#include <span>

namespace tyr
{

namespace
{
using Column = urd::index<ygg::Column>;
using Relation = urd::builder<ygg::Relation<ygg::Schema>>;
using ColumnIndices = std::span<const ygg::ColumnLayout>;
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
        .def("input_columns", &ProjectionPlan::input_columns, nb::keep_alive<0, 1>())
        .def("output_columns", &ProjectionPlan::output_columns, nb::keep_alive<0, 1>());
    nb::class_<JoinPlan>(m, "JoinPlan", "Natural join (also semijoin, antijoin) of inputs with these columns.")
        .def(nb::init<ColumnIndices, ColumnIndices>(), nb::arg("lhs"), nb::arg("rhs"))
        .def("lhs_columns", &JoinPlan::lhs_columns, nb::keep_alive<0, 1>())
        .def("rhs_columns", &JoinPlan::rhs_columns, nb::keep_alive<0, 1>())
        .def("output_columns", &JoinPlan::output_columns, nb::keep_alive<0, 1>());
    nb::class_<SetPlan>(m, "SetPlan", "Union and difference of inputs with the same columns in the same order.")
        .def(nb::init<ColumnIndices, ColumnIndices>(), nb::arg("lhs"), nb::arg("rhs"))
        .def("output_columns", &SetPlan::output_columns, nb::keep_alive<0, 1>());
    nb::class_<SelectEqualPlan>(m, "SelectEqualPlan", "The rows whose two columns (of one type) hold equal values.")
        .def(nb::init<ColumnIndices, Column, Column>(), nb::arg("input"), nb::arg("lhs"), nb::arg("rhs"))
        .def("output_columns", &SelectEqualPlan::output_columns, nb::keep_alive<0, 1>());
    nb::class_<SelectValuePlan>(m, "SelectValuePlan", "The rows whose column holds a value of the column's type.")
        .def(
            "__init__",
            [](SelectValuePlan* self, ColumnIndices input, Column column, ygg::TypeId type, nb::handle value)
            {
                new (self) SelectValuePlan(input, column, type, ygg::encode(ygg::cast_value(value, type)));
            },
            nb::arg("input"),
            nb::arg("column"),
            nb::arg("type"),
            nb::arg("value"))
        .def("output_columns", &SelectValuePlan::output_columns, nb::keep_alive<0, 1>());
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
            require_columns(out.columns(), plan.output_columns());
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
            require_columns(out.columns(), plan.output_columns());
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
            require_columns(out.columns(), plan.output_columns());
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
    const auto bind_join = [&m](const char* name, auto operation, bool output_is_lhs)
    {
        m.def(
            name,
            [operation, output_is_lhs](const L& lhs, const R& rhs, const JoinPlan& plan, Relation& out, Workspace& workspace)
            {
                require_columns(lhs.columns(), plan.lhs_columns());
                require_columns(rhs.columns(), plan.rhs_columns());
                require_columns(out.columns(), output_is_lhs ? plan.lhs_columns() : plan.output_columns());
                operation(lhs, rhs, plan, out, workspace);
            },
            nb::arg("lhs"),
            nb::arg("rhs"),
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
            [operation](const L& lhs, const R& rhs, const SetPlan& plan, Relation& out)
            {
                for (const auto columns : { lhs.columns(), rhs.columns(), out.columns() })
                    require_columns(columns, plan.output_columns());
                operation(lhs, rhs, plan, out);
            },
            nb::arg("lhs"),
            nb::arg("rhs"),
            nb::arg("plan"),
            nb::arg("out"));
    };
    bind_set("union", [](auto&&... args) { union_(args...); });
    bind_set("difference", [](auto&&... args) { difference(args...); });
}
}  // namespace

void bind_database_module_definitions(nb::module_& m)
{
    using RelationView = ygg::FreeRelationView;
    using BuilderRelationView = ygg::FreeBuilderRelationView;
    bind_plans(m);
    bind_unary<RelationView>(m);
    bind_unary<BuilderRelationView>(m);
    bind_binary<RelationView, RelationView>(m);
    bind_binary<RelationView, BuilderRelationView>(m);
    bind_binary<BuilderRelationView, RelationView>(m);
    bind_binary<BuilderRelationView, BuilderRelationView>(m);
    bind_database_queries(m);
    bind_database_optimization(m);
    bind_database_evaluation(m);
    bind_database_distance(m);
}

}  // namespace tyr
