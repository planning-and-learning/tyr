#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_FDR_TASK_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_PLANNING_FDR_TASK_HPP_

#include "tyr/formalism/planning/planning_fdr_task.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/fdr_task_view.hpp"
#include "tyr/serialization/formalism/planning/planning_domain.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::PlanningFDRTask>)
{
    ar.field("task", [](const auto& value) -> decltype(auto) { return (value.get_task()); });
    ar.field("domain", [](const auto& value) -> decltype(auto) { return (value.get_domain()); });
    ar.field("path", [](const auto& value) -> decltype(auto) { return (value.get_path()); });
}

}

#endif
