#ifndef TYR_SERIALIZATION_PLANNING_LIFTED_TASK_HPP_
#define TYR_SERIALIZATION_PLANNING_LIFTED_TASK_HPP_

#include "tyr/planning/lifted/task.hpp"
#include "tyr/serialization/formalism/planning/planning_task.hpp"
#include "tyr/serialization/serializer.hpp"

#include <string>

namespace tyr::serialization
{

template<>
struct Serializer<planning::Task<LiftedTag>>
{
    static std::string name() { return "LiftedTask"; }

    template<class Archive>
    static void save(Archive& archive, const planning::Task<LiftedTag>& task)
    {
        archive.field("formalism_task", task.get_formalism_task());
    }
};

}

#endif
