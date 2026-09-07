#ifndef TYR_SERIALIZATION_FORMALISM_FUNCTION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_FUNCTION_VIEW_HPP_

#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::formalism::FactKind T>
struct TypeName<::tyr::formalism::planning::FunctionView<T>>
{
    static std::string get() { return std::string(T::name) + "Function"; }
};

template<class Archive, ::tyr::formalism::FactKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::FunctionView<T>>)
{
    ar.field("name", [](const auto& value) -> decltype(auto) { return (value.get_name()); });
    ar.field("arity", [](const auto& value) -> decltype(auto) { return (value.get_arity()); });
}

}

#endif
