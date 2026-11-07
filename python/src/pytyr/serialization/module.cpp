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
#include <yggdrasil/serialization/dictionaries.hpp>
#include <tyr/serialization/types.hpp>
#include <yggdrasil/python/serialization.hpp>

namespace nb = nanobind;

namespace tyr::serialization
{
void bind_module_definitions(nb::module_& m) { ygg::python::bind_serialization(m, RegisteredTypes {}, SerializedTypes {}, ProjectionTypes {}); }

}  // namespace tyr::serialization
