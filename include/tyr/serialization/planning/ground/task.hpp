#ifndef TYR_SERIALIZATION_PLANNING_GROUND_TASK_HPP_
#define TYR_SERIALIZATION_PLANNING_GROUND_TASK_HPP_

#include "tyr/planning/ground/task.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/planning/planning_fdr_task.hpp"

#include <string>

namespace tyr::serialization
{

template<>
struct TypeName<planning::Task<GroundTag>>
{
    static std::string get() { return "GroundTask"; }
};

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const planning::Task<GroundTag>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar) { ar.field("formalism_task", value.get_formalism_task()); });
}

}

#endif
