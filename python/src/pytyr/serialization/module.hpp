#ifndef PYTYR_SERIALIZATION_MODULE_HPP_
#define PYTYR_SERIALIZATION_MODULE_HPP_

#include <nanobind/nanobind.h>

namespace tyr::serialization
{
void bind_module_definitions(nanobind::module_& m);
}

#endif
