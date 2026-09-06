#ifndef TYR_SERIALIZATION_PLANNING_LIFTED_TASK_HPP_
#define TYR_SERIALIZATION_PLANNING_LIFTED_TASK_HPP_

#include "tyr/planning/lifted/task.hpp"
#include "tyr/serialization/formalism/planning/planning_task.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

#include <string>

namespace ygg::serialization
{

template<>
struct TypeName<::tyr::planning::Task<::tyr::LiftedTag>>
{
    static std::string get() { return "LiftedTask"; }
};

inline void
tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::planning::Task<::tyr::LiftedTag>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar) { ar.field("formalism_task", value.get_formalism_task()); });
}

}

#endif
