#ifndef TYR_SERIALIZATION_FORMALISM_BINDING_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_BINDING_VIEW_HPP_

#include "tyr/formalism/binding_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/object_view.hpp"
#include "tyr/serialization/formalism/planning/action_view.hpp"
#include "tyr/serialization/formalism/planning/axiom_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive, typename T>
void describe_fields(Archive& ar, std::type_identity<ygg::View<ygg::Index<::tyr::formalism::RelationBinding<T>>, ::tyr::formalism::planning::Repository>>)
{
    ar.field("relation", [](const auto& value) -> decltype(auto) { return (value.get_relation()); });
    ar.field("objects", [](const auto& value) -> decltype(auto) { return (value.get_objects()); });
}

}

#endif
