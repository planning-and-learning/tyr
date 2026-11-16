#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_empty(nb::module_& m, QueryRepositoryBinding& repository)
{
    urd::python::bind_data<tyr::datalog::database::Query<tyr::datalog::database::QueryEmptyTag>>(m);
    bind_query_operation<tyr::datalog::database::QueryEmptyTag>(m, repository);
}

}  // namespace tyr::database_python
