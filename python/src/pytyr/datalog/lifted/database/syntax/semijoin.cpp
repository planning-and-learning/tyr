#include "bindings.hpp"
#include "pytyr/module.hpp"

namespace tyr::database_python
{
void bind_query_semijoin(nb::module_& m, QueryRepositoryBinding& repository)
{
    urd::python::bind_index<tyr::datalog::database::Query<tyr::datalog::database::QuerySemijoinTag>>(m);
    urd::python::bind_data<tyr::datalog::database::Query<tyr::datalog::database::QuerySemijoinTag>>(m);
    urd::python::bind_builder<tyr::datalog::database::Query<tyr::datalog::database::QuerySemijoinTag>, tyr::datalog::database::QueryRepository>(m);
    bind_query_operation<tyr::datalog::database::QuerySemijoinTag>(m, repository);
}

}  // namespace tyr::database_python
