#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_distance(nb::module_& m, QueryRepositoryBinding& repository)
{
    urd::python::bind_data<tyr::datalog::database::Query<tyr::datalog::database::QueryDistanceTag>>(m);
    bind_query_operation<tyr::datalog::database::QueryDistanceTag>(m, repository);
}

}  // namespace tyr::database_python
