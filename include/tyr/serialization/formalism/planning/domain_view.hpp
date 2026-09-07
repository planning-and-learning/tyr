#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_DOMAIN_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_DOMAIN_VIEW_HPP_

#include "tyr/formalism/planning/domain_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/object_view.hpp"
#include "tyr/serialization/formalism/planning/action_view.hpp"
#include "tyr/serialization/formalism/planning/axiom_view.hpp"
#include "tyr/serialization/formalism/predicate_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::DomainView>)
{
    ar.field("name", [](const auto& value) -> decltype(auto) { return (value.get_name()); });
    ar.field("static_predicates", [](const auto& value) -> decltype(auto) { return (value.template get_predicates<::tyr::formalism::StaticTag>()); });
    ar.field("fluent_predicates", [](const auto& value) -> decltype(auto) { return (value.template get_predicates<::tyr::formalism::FluentTag>()); });
    ar.field("derived_predicates", [](const auto& value) -> decltype(auto) { return (value.template get_predicates<::tyr::formalism::DerivedTag>()); });
    ar.field("static_functions", [](const auto& value) -> decltype(auto) { return (value.template get_functions<::tyr::formalism::StaticTag>()); });
    ar.field("fluent_functions", [](const auto& value) -> decltype(auto) { return (value.template get_functions<::tyr::formalism::FluentTag>()); });
    ar.field("auxiliary_function", [](const auto& value) -> decltype(auto) { return (value.get_auxiliary_function()); });
    ar.field("constants", [](const auto& value) -> decltype(auto) { return (value.get_constants()); });
    ar.field("actions", [](const auto& value) -> decltype(auto) { return (value.get_actions()); });
    ar.field("axioms", [](const auto& value) -> decltype(auto) { return (value.get_axioms()); });
}

}

#endif
