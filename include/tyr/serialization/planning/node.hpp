#ifndef TYR_SERIALIZATION_PLANNING_NODE_HPP_
#define TYR_SERIALIZATION_PLANNING_NODE_HPP_

#include "tyr/planning/node.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/planning/state_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind Kind>
void describe_fields(Archive& ar, std::type_identity<::tyr::planning::Node<::tyr::planning::StateView<Kind>>>)
{
    ar.field("state", [](const auto& value) -> decltype(auto) { return (value.get_state()); });
    ar.field("metric", [](const auto& value) -> decltype(auto) { return (value.get_metric()); });
}

template<class Archive, ::tyr::TaskKind Kind>
void describe_fields(Archive& ar, std::type_identity<::tyr::planning::LabeledNode<::tyr::planning::StateView<Kind>>>)
{
    ar.field("label", [](const auto& value) -> decltype(auto) { return (value.label); });
    ar.field("node", [](const auto& value) -> decltype(auto) { return (value.node); });
}

}

#endif
