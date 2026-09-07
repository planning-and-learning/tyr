#include "module.hpp"

#include <type_traits>
#include <tyr/serialization/serialization.hpp>
#include <tyr/serialization/types.hpp>
#include <yggdrasil/core/concepts.hpp>
#include <yggdrasil/python/serialization.hpp>

namespace nb = nanobind;

namespace tyr::serialization
{
namespace
{
template<typename T>
using HashableTypeList = std::conditional_t<ygg::Hashable<T>, ygg::TypeList<T>, ygg::TypeList<>>;

using RegisteredTypes = ygg::ApplyTypeListT<ygg::ConcatTypeListsT, ygg::MapTypeListT<HashableTypeList, SerializedTypes>>;

}  // namespace

void bind_module_definitions(nb::module_& m)
{
    ygg::python::bind_serialization(m, RegisteredTypes {}, SerializedTypes {}, ProjectionTypes {});
}

}  // namespace tyr::serialization
