#include "bindings.hpp"
#include "pytyr/module.hpp"

#include <urd/python/sequences/sequence.hpp>
#include <urd/repr/sequences/sequence.hpp>
#include <urd/store.hpp>

namespace tyr::database_python
{
void bind_query_project(nb::module_& m, QueryRepositoryBinding& repository)
{
    urd::python::bind_data<tyr::datalog::database::Query<tyr::datalog::database::QueryProjectTag>>(m);
    bind_query_operation<tyr::datalog::database::QueryProjectTag>(m, repository);
}

}  // namespace tyr::database_python
