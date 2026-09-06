#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_DOMAIN_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_DOMAIN_HPP_

#include "tyr/formalism/planning/planning_domain.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/planning/domain_view.hpp"

namespace tyr::serialization
{

template<>
struct TypeName<formalism::planning::PlanningDomain>
{
    static std::string get() { return "PlanningDomain"; }
};

inline void tag_invoke(boost::json::value_from_tag,
                       boost::json::value& result,
                       const formalism::planning::PlanningDomain& value,
                       Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        ar.field("domain", value.get_domain());
        ar.field("path", value.get_path());
    });
}

}

#endif
