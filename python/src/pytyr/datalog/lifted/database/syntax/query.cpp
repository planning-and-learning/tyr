#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query(nb::module_& m, QueryRepositoryBinding& repository)
{
    using Query = tyr::datalog::database::Query<>;
    urd::python::bind_data<Query>(m);
    urd::python::bind_view<QueryView>(m).def("columns", &QueryView::columns);
    ygg::bind_insert<Query>(repository);
}

}  // namespace tyr::database_python

namespace tyr
{
void bind_database_queries(nb::module_& m)
{
    auto repository = database_python::QueryRepositoryBinding(m, "QueryRepository", "Interns validated relational expressions without reading data.");
    repository.def(nb::init<>()).def("__len__", [](const tyr::datalog::database::QueryRepository& self) { return self.size(); });
    database_python::bind_query_input(m, repository);
    database_python::bind_query_empty(m, repository);
    database_python::bind_query_join(m, repository);
    database_python::bind_query_project(m, repository);
    database_python::bind_query_rename(m, repository);
    database_python::bind_query_select_equal(m, repository);
    database_python::bind_query_select_value(m, repository);
    database_python::bind_query_union(m, repository);
    database_python::bind_query_difference(m, repository);
    database_python::bind_query_distance(m, repository);
    database_python::bind_query_generic_join(m, repository);
    database_python::bind_query_semijoin(m, repository);
    database_python::bind_query_antijoin(m, repository);
    database_python::bind_query(m, repository);
}
}  // namespace tyr
