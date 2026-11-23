#ifndef PYTYR_SERIALIZATION_MODULE_HPP_
#define PYTYR_SERIALIZATION_MODULE_HPP_

#include <cstddef>
#include <nanobind/nanobind.h>

namespace tyr::serialization
{
/// The sources the bindings are split over (module_part_<k>.cpp), so that no one compiles them all.
inline constexpr std::size_t module_parts = 4;

/// The bindings of the Part-th slice of the types: one source instantiates each.
template<std::size_t Part>
void bind_module_part(nanobind::module_& m);

void bind_module_definitions(nanobind::module_& m);
}

#endif
