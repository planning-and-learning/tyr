#ifndef PYTYR_DATABASE_MODULE_HPP_
#define PYTYR_DATABASE_MODULE_HPP_

#include "tyr/datalog/lifted/database/semantics/relation_view.hpp"
#include "yggdrasil/formalism/free_relations.hpp"
#include "yggdrasil/formalism/relation_builder.hpp"
#include "yggdrasil/python/relations.hpp"

#include <algorithm>
#include <nanobind/nanobind.h>
#include <span>
#include <stdexcept>

namespace tyr
{
namespace nb = nanobind;

/// Python is a trust boundary: the operations only assert (debug builds) that their relations have the plan's columns,
/// so the bindings check it, throwing ValueError.
inline void require_columns(std::span<const ygg::ColumnLayout> actual, std::span<const ygg::ColumnLayout> expected)
{
    if (!std::ranges::equal(actual, expected))
        throw std::invalid_argument("Relational operation: columns do not match the plan.");
}

/// The relational operations, plans, queries and their evaluation over pyyggdrasil.formalism's free relations.
void bind_database_module_definitions(nb::module_& m);
void bind_database_datalog(nb::module_& m);
void bind_database_evaluation(nb::module_& m);
void bind_database_optimization(nb::module_& m);
void bind_database_queries(nb::module_& m);

}  // namespace tyr

#endif
