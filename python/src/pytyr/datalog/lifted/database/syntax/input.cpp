#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_input(nb::module_& m, QueryRepositoryBinding& repository)
{
    urd::python::bind_data<tyr::datalog::database::Query<tyr::datalog::database::QueryInputTag>>(m);
    bind_query_operation<tyr::datalog::database::QueryInputTag>(m, repository);
}

}  // namespace tyr::database_python
