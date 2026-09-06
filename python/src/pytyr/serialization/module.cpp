#include "module.hpp"

#include <nanobind/stl/string.h>
#include <type_traits>
#include <tyr/serialization/serialization.hpp>
#include <tyr/serialization/types.hpp>
#include <yggdrasil/core/concepts.hpp>

namespace nb = nanobind;
using namespace nb::literals;

namespace tyr::serialization
{
namespace
{
nb::object to_python(const boost::json::value& value)
{
    switch (value.kind())
    {
        case boost::json::kind::null: return nb::none();
        case boost::json::kind::bool_: return nb::bool_(value.as_bool());
        case boost::json::kind::int64: return nb::int_(value.as_int64());
        case boost::json::kind::uint64: return nb::int_(value.as_uint64());
        case boost::json::kind::double_: return nb::float_(value.as_double());
        case boost::json::kind::string:
        {
            const auto& text = value.as_string();
            return nb::str(text.data(), text.size());
        }
        case boost::json::kind::array:
        {
            auto result = nb::list();
            for (const auto& item : value.as_array())
                result.append(to_python(item));
            return result;
        }
        case boost::json::kind::object:
        {
            auto result = nb::dict();
            for (const auto& item : value.as_object())
                result[nb::str(item.key().data(), item.key().size())] = to_python(item.value());
            return result;
        }
    }
    throw nb::type_error("unsupported JSON value");
}

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

template<typename... Ts>
void register_table(Dictionaries& self, nb::type_object native_type, const std::string& name, const std::string& prefix, ygg::TypeList<Ts...>)
{
    if (!((native_type.is(nb::type<Ts>()) && (self.register_table<Ts>(name, prefix), true)) || ...))
        throw nb::type_error("this native type cannot be registered as a table");
}

template<typename... Ts>
nb::object serialize(Dictionaries& self, nb::handle value, ygg::TypeList<Ts...>)
{
    nb::object result;
    if (!((nb::isinstance<Ts>(value) && (result = to_python(self.serialize(nb::cast<const Ts&>(value))), true)) || ...))
        throw nb::type_error("this native type does not support serialization");
    return result;
}

template<typename... Ts>
nb::object table(Dictionaries& self, nb::type_object native_type, ygg::TypeList<Ts...>)
{
    nb::object result;
    if (!((native_type.is(nb::type<Ts>()) && (result = to_python(self.table<Ts>()), true)) || ...))
        throw nb::type_error("this native type cannot be registered as a table");
    return result;
}
}

void bind_module_definitions(nb::module_& m)
{
    nb::class_<Dictionaries>(m, "Dictionaries")
        .def(nb::init<>())
        .def("register_table",
             [](Dictionaries& self, nb::type_object native_type, const std::string& name, const std::string& prefix)
             { register_table(self, native_type, name, prefix, RegisteredTypes {}); },
             "native_type"_a,
             "name"_a,
             "prefix"_a)
        .def("serialize",
             [](Dictionaries& self, nb::handle value) { return serialize(self, value, SerializedTypes {}); },
             "value"_a,
             nb::keep_alive<1, 2>())
        .def("table",
             [](Dictionaries& self, nb::type_object native_type) { return table(self, native_type, RegisteredTypes {}); },
             "native_type"_a)
        .def("tables", [](Dictionaries& self) { return to_python(self.tables()); })
        .def("enums", [](Dictionaries& self) { return to_python(self.enums()); });
}
}
