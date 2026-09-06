#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_FDR_TASK_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_FDR_TASK_HPP_

#include "tyr/formalism/planning/planning_fdr_task.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/fdr_task_view.hpp"
#include "tyr/serialization/formalism/planning/planning_domain.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<>
struct TypeName<::tyr::formalism::planning::PlanningFDRTask>
{
    static std::string get() { return "PlanningFDRTask"; }
};

inline void
tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::planning::PlanningFDRTask& value, Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("task", value.get_task());
                             ar.field("domain", value.get_domain());
                             ar.field("path", value.get_path());
                         });
}

}

#endif
