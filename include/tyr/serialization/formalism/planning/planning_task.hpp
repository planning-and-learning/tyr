#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_TASK_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_TASK_HPP_

#include "tyr/formalism/planning/planning_task.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/planning/planning_domain.hpp"
#include "tyr/serialization/formalism/planning/task_view.hpp"

namespace tyr::serialization
{

template<>
struct TypeName<formalism::planning::PlanningTask>
{
    static std::string get() { return "PlanningTask"; }
};

inline void tag_invoke(boost::json::value_from_tag,
                       boost::json::value& result,
                       const formalism::planning::PlanningTask& value,
                       Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        ar.field("task", value.get_task());
        ar.field("domain", value.get_domain());
        ar.field("path", value.get_path());
    });
}

}

#endif
