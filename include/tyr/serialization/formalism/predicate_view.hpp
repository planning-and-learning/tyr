#ifndef TYR_SERIALIZATION_FORMALISM_PREDICATE_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PREDICATE_VIEW_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/predicate_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::formalism::FactKind T>
struct TypeName<::tyr::formalism::planning::PredicateView<T>>
{
    static std::string get() { return std::string(T::name) + "Predicate"; }
};

template<class Archive, ::tyr::formalism::FactKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::PredicateView<T>>)
{
    ar.field("name", [](const auto& value) -> decltype(auto) { return (value.get_name()); });
    ar.field("arity", [](const auto& value) -> decltype(auto) { return (value.get_arity()); });
}

}

#endif
