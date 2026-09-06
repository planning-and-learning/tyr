#ifndef TYR_SERIALIZATION_PLANNING_LIFTED_TASK_HPP_
#define TYR_SERIALIZATION_PLANNING_LIFTED_TASK_HPP_

#include "tyr/planning/lifted/task.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/planning/planning_task.hpp"

#include <string>

namespace tyr::serialization
{

template<>
struct TypeName<planning::Task<LiftedTag>>
{
    static std::string get() { return "LiftedTask"; }
};

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const planning::Task<LiftedTag>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar) { ar.field("formalism_task", value.get_formalism_task()); });
}

}

#endif
