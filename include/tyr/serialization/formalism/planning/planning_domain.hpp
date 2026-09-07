#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_DOMAIN_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_DOMAIN_HPP_

#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/domain_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::PlanningDomain>)
{
    ar.field("domain", [](const auto& value) -> decltype(auto) { return (value.get_domain()); });
    ar.field("path", [](const auto& value) -> decltype(auto) { return (value.get_path()); });
}

}

#endif
