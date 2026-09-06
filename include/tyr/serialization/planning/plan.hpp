#ifndef TYR_SERIALIZATION_PLANNING_PLAN_HPP_
#define TYR_SERIALIZATION_PLANNING_PLAN_HPP_

#include "tyr/planning/plan.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/planning/node.hpp"

#include <string>

namespace tyr::serialization
{

template<TaskKind T>
struct TypeName<planning::Plan<T>>
{
    static std::string get() { return std::string(T::name) + "Plan"; }
};

template<TaskKind T>
void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const planning::Plan<T>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("start_node", value.get_start_node());
                             ar.field("labeled_succ_nodes", value.get_labeled_succ_nodes());
                             ar.field("length", value.get_length());
                             ar.field("cost", value.get_cost());
                         });
}

}

#endif
