#ifndef TYR_SERIALIZATION_FORMALISM_VARIABLE_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_VARIABLE_VIEW_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<>
struct TypeName<::tyr::formalism::planning::VariableView>
{
    static std::string get() { return "Variable"; }
};

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::VariableView>)
{
    ar.field("name", [](const auto& value) -> decltype(auto) { return (value.get_name()); });
}

}

#endif
