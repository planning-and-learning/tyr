#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_select_value(nb::module_& m, QueryRepositoryBinding& repository)
{
    urd::python::bind_index<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectValueTag>>(m);
    urd::python::bind_data<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectValueTag>>(m);
    urd::python::bind_builder<tyr::datalog::database::Query<tyr::datalog::database::QuerySelectValueTag>, tyr::datalog::database::QueryRepository>(m);
    bind_query_operation<tyr::datalog::database::QuerySelectValueTag>(m, repository);
}

}  // namespace tyr::database_python
