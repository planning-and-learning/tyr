#ifndef TYR_SERIALIZATION_PLANNING_PLAN_HPP_
#define TYR_SERIALIZATION_PLANNING_PLAN_HPP_

#include "tyr/planning/plan.hpp"
#include "tyr/serialization/planning/node.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::planning::Plan<T>>)
{
    ar.field("start_node", [](const auto& value) -> decltype(auto) { return (value.get_start_node()); });
    ar.field("labeled_succ_nodes", [](const auto& value) -> decltype(auto) { return (value.get_labeled_succ_nodes()); });
    ar.field("length", [](const auto& value) -> decltype(auto) { return (value.get_length()); });
    ar.field("cost", [](const auto& value) -> decltype(auto) { return (value.get_cost()); });
}

}

#endif
