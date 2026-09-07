#ifndef TYR_SERIALIZATION_FORMALISM_OBJECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_OBJECT_VIEW_HPP_

#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<>
struct TypeName<::tyr::formalism::planning::ObjectView>
{
    static std::string get() { return "Object"; }
};

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::ObjectView>)
{
    ar.field("name", [](const auto& value) -> decltype(auto) { return (value.get_name()); });
}

}

#endif
