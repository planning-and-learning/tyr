#ifndef TYR_SERIALIZATION_PLANNING_PLAN_HPP_
#define TYR_SERIALIZATION_PLANNING_PLAN_HPP_

#include "tyr/planning/plan.hpp"
#include "tyr/serialization/planning/node.hpp"

#include <string>

namespace tyr::serialization
{

template<TaskKind Kind>
struct Serializer<planning::Plan<Kind>>
{
    static std::string name() { return std::same_as<Kind, GroundTag> ? "GroundPlan" : "LiftedPlan"; }

    template<class Archive>
    static void save(Archive& archive, const planning::Plan<Kind>& plan)
    {
        archive.field("start_node", plan.get_start_node());
        archive.field("labeled_succ_nodes", plan.get_labeled_succ_nodes());
        archive.field("length", plan.get_length());
        archive.field("cost", plan.get_cost());
    }
};

}

#endif
