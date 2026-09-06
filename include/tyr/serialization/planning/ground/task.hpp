#ifndef TYR_SERIALIZATION_PLANNING_GROUND_TASK_HPP_
#define TYR_SERIALIZATION_PLANNING_GROUND_TASK_HPP_

#include "tyr/planning/ground/task.hpp"
#include "tyr/serialization/formalism/planning/planning_fdr_task.hpp"
#include "tyr/serialization/serializer.hpp"

#include <string>

namespace tyr::serialization
{

template<>
struct Serializer<planning::Task<GroundTag>>
{
    static std::string name() { return "GroundTask"; }

    template<class Archive>
    static void save(Archive& archive, const planning::Task<GroundTag>& task)
    {
        archive.field("formalism_task", task.get_formalism_task());
    }
};

}

#endif
