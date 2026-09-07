#include "module.hpp"

#include <tyr/serialization/serialization.hpp>
#include <tyr/serialization/types.hpp>
#include <yggdrasil/python/serialization.hpp>

namespace nb = nanobind;

namespace tyr::serialization
{
void bind_module_definitions(nb::module_& m)
{
    ygg::python::bind_serialization(m, RegisteredTypes {}, SerializedTypes {}, ProjectionTypes {});
}

}  // namespace tyr::serialization
