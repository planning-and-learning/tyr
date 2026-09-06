#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_FDR_TASK_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_FDR_TASK_HPP_

#include "tyr/formalism/planning/planning_fdr_task.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/fdr_task_view.hpp"
#include "tyr/serialization/formalism/planning/planning_domain.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<formalism::planning::PlanningFDRTask>
{
    static std::string name() { return "PlanningFDRTask"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::PlanningFDRTask& value)
    {
        ar.field("task", value.get_task());
        ar.field("domain", value.get_domain());
        ar.field("path", value.get_path());
    }
};

}

#endif
