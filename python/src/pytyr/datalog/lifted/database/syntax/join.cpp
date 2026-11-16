#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_join(nb::module_& m, QueryRepositoryBinding& repository)
{
    urd::python::bind_data<tyr::datalog::database::Query<tyr::datalog::database::QueryJoinTag>>(m);
    bind_query_operation<tyr::datalog::database::QueryJoinTag>(m, repository);
}

}  // namespace tyr::database_python
