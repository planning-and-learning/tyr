#ifndef TYR_SERIALIZATION_PLANNING_NODE_HPP_
#define TYR_SERIALIZATION_PLANNING_NODE_HPP_

#include "tyr/planning/node.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/planning/state_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

#include <string>

namespace ygg::serialization
{

template<::tyr::TaskKind T>
struct TypeName<::tyr::planning::Node<T>>
{
    static std::string get() { return std::string(T::name) + "Node"; }
};

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::planning::Node<T>>)
{
    ar.field("state", [](const auto& value) -> decltype(auto) { return (value.get_state()); });
    ar.field("metric", [](const auto& value) -> decltype(auto) { return (value.get_metric()); });
}

template<::tyr::TaskKind T>
struct TypeName<::tyr::planning::LabeledNode<T>>
{
    static std::string get() { return std::string(T::name) + "LabeledNode"; }
};

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::planning::LabeledNode<T>>)
{
    ar.field("label", [](const auto& value) -> decltype(auto) { return (value.label); });
    ar.field("node", [](const auto& value) -> decltype(auto) { return (value.node); });
}

}

#endif
