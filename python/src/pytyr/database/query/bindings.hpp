#ifndef PYTYR_DATABASE_QUERY_BINDINGS_HPP_
#define PYTYR_DATABASE_QUERY_BINDINGS_HPP_

#include "../module.hpp"
#include "pytyr/module.hpp"
#include "tyr/datalog/lifted/database/syntax/query.hpp"
#include "yggdrasil/python/bindings.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

namespace tyr::database_python
{
namespace nb = nanobind;

using Column = urd::index<ygg::Column>;
using ColumnIndices = std::span<const ygg::ColumnLayout>;
using QueryView = tyr::datalog::database::QueryView<>;
using QueryRepositoryBinding = nb::class_<tyr::datalog::database::QueryRepository>;

template<typename Tag>
using QueryData = urd::data<tyr::datalog::database::Query<Tag>>;
template<typename Tag>
using ConcreteQueryView = tyr::datalog::database::QueryView<Tag>;

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
void bind_query_generic_join(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_semijoin(nb::module_& m, QueryRepositoryBinding& repository);
void bind_query_antijoin(nb::module_& m, QueryRepositoryBinding& repository);

}  // namespace tyr::database_python

#endif
