#include "module.hpp"

namespace tyr::serialization
{
void bind_module_definitions(nanobind::module_& m)
{
    bind_module_part<0>(m);
    bind_module_part<1>(m);
    bind_module_part<2>(m);
    bind_module_part<3>(m);
}

}  // namespace tyr::serialization
