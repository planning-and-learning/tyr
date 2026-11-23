#ifndef TYR_SERIALIZATION_PLANNING_LIFTED_TASK_HPP_
#define TYR_SERIALIZATION_PLANNING_LIFTED_TASK_HPP_

#include "tyr/declarations.hpp"
#include "tyr/planning/lifted/task.hpp"

#include <type_traits>

namespace ygg::serialization
{

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::planning::Task<::tyr::LiftedTag>>)
{
    ar.field("formalism_task", [](const auto& value) -> decltype(auto) { return (value.get_formalism_task()); });
}

}

#endif
