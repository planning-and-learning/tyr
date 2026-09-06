#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_DOMAIN_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_DOMAIN_HPP_

#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/domain_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::PlanningDomain>
{
    static std::string name() { return "PlanningDomain"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::PlanningDomain& value)
    {
        ar.field("domain", value.get_domain());
        ar.field("path", value.get_path());
    }
};

}

#endif
