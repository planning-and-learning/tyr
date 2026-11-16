#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_difference(nb::module_& m, QueryRepositoryBinding& repository)
{
    urd::python::bind_data<tyr::datalog::database::Query<tyr::datalog::database::QueryDifferenceTag>>(m);
    bind_query_operation<tyr::datalog::database::QueryDifferenceTag>(m, repository);
}

}  // namespace tyr::database_python
