#ifndef PYTYR_DATABASE_QUERY_BINDINGS_HPP_
#define PYTYR_DATABASE_QUERY_BINDINGS_HPP_

#include "../module.hpp"
#include "pytyr/module.hpp"
#include "tyr/datalog/lifted/database/syntax/formatter.hpp"
#include "tyr/datalog/lifted/database/syntax/query.hpp"
#include "tyr/datalog/lifted/database/syntax/query_construction.hpp"
#include "yggdrasil/python/bindings.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>
#include <urd/python/bindings.hpp>

namespace tyr::database_python
{
namespace nb = nanobind;

using Column = urd::index<ygg::Column>;
using ColumnIndices = std::span<const ygg::ColumnLayout>;
using QueryView = tyr::datalog::database::QueryView<>;
using QueryRepositoryBinding = nb::class_<tyr::datalog::database::QueryRepository>;

template<typename Tag>
using ConcreteQueryView = tyr::datalog::database::QueryView<Tag>;

/// A query of operation Tag, whose record is bound first (bind_data; Python builds it through its empty __init__ and
/// its fields): its view (bind_view, and its columns), and QueryRepository.insert of the record, prepared first: its plan
/// derived from its children (prepare_for_insert).
template<typename Tag>
nb::class_<ConcreteQueryView<Tag>> bind_query_operation(nb::module_& m, QueryRepositoryBinding& repository)
{
    repository.def(
        "insert",
        [](tyr::datalog::database::QueryRepository& self, urd::data<tyr::datalog::database::Query<Tag>>& data)
        { return tyr::datalog::database::insert(self, data); },
        nb::arg("data"));
    return urd::python::bind_view<ConcreteQueryView<Tag>>(m);
}

void bind_query(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_input(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_empty(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_join(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_project(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_rename(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_select_equal(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_select_value(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_union(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_difference(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_distance(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_generic_join(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_semijoin(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_antijoin(nb::module_& m, QueryRepositoryBinding& repository);

}  // namespace tyr::database_python

#endif
