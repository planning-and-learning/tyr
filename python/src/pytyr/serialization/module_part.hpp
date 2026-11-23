#ifndef PYTYR_SERIALIZATION_MODULE_PART_HPP_
#define PYTYR_SERIALIZATION_MODULE_PART_HPP_

// The serialization bindings of one slice of tyr's types (module.hpp): each module_part_<k>.cpp instantiates one.

#include "module.hpp"
#include "pytyr/module.hpp"

#include <tyr/formalism/planning/formatter.hpp>
#include <tyr/planning/formatter.hpp>
#include <tyr/serialization/formalism/enums.hpp>
#include <tyr/serialization/formalism/planning/planning.hpp>
#include <tyr/serialization/planning/ground/task.hpp>
#include <tyr/serialization/planning/lifted/task.hpp>
#include <tyr/serialization/planning/node.hpp>
#include <tyr/serialization/planning/plan.hpp>
#include <tyr/serialization/planning/state_view.hpp>
#include <tyr/serialization/types.hpp>
#include <yggdrasil/python/serialization.hpp>
#include <yggdrasil/serialization/dictionaries.hpp>

namespace tyr::serialization
{
template<std::size_t Part>
void bind_module_part(nanobind::module_& m)
{ ygg::python::bind_serialization_part<Part, module_parts>(m, RegisteredTypes {}, SerializedTypes {}, ProjectionTypes {}); }
}  // namespace tyr::serialization

#endif
