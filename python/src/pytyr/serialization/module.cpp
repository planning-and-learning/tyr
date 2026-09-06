#include "module.hpp"

#include <nanobind/stl/string.h>
#include <type_traits>
#include <tyr/serialization/serialization.hpp>
#include <tyr/serialization/types.hpp>
#include <yggdrasil/core/concepts.hpp>
#include <yggdrasil/python/serialization.hpp>

namespace nb = nanobind;
using namespace nb::literals;

namespace tyr::serialization
{
using ygg::serialization::Dictionaries;

namespace
{
using RuntimeStates = ygg::TypeList<planning::StateView<GroundTag>, planning::StateView<LiftedTag>>;
using RuntimeOwners = ygg::TypeList<planning::Task<GroundTag>,
                                    planning::Task<LiftedTag>,
                                    planning::Node<GroundTag>,
                                    planning::Node<LiftedTag>,
                                    planning::LabeledNode<GroundTag>,
                                    planning::LabeledNode<LiftedTag>,
                                    planning::Plan<GroundTag>,
                                    planning::Plan<LiftedTag>>;
using SerializedTypes = ygg::ConcatTypeListsT<FormalismViews, RuntimeStates, FormalismOwners, RuntimeOwners>;

template<typename T>
using HashableTypeList = std::conditional_t<ygg::Hashable<T>, ygg::TypeList<T>, ygg::TypeList<>>;

using RegisteredTypes = ygg::ApplyTypeListT<ygg::ConcatTypeListsT, ygg::MapTypeListT<HashableTypeList, SerializedTypes>>;

}  // namespace

void bind_module_definitions(nb::module_& m)
{
    nb::class_<Dictionaries>(m, "Dictionaries")
        .def(nb::init<>())
        .def(
            "register_table",
            [](Dictionaries& self, nb::type_object native_type, const std::string& name, const std::string& prefix)
            { ygg::python::register_table(self, native_type, name, prefix, RegisteredTypes {}); },
            "native_type"_a,
            "name"_a,
            "prefix"_a)
        .def(
            "serialize",
            [](Dictionaries& self, nb::handle value) { return ygg::python::serialize(self, value, SerializedTypes {}); },
            "value"_a,
            nb::keep_alive<1, 2>())
        .def(
            "table",
            [](Dictionaries& self, nb::type_object native_type) { return ygg::python::table(self, native_type, RegisteredTypes {}); },
            "native_type"_a)
        .def("tables", [](Dictionaries& self) { return ygg::python::to_python(self.tables()); })
        .def("enums", [](Dictionaries& self) { return ygg::python::to_python(self.enums()); });
}
}
