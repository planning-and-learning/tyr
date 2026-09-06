#ifndef TYR_SERIALIZATION_PLANNING_NODE_HPP_
#define TYR_SERIALIZATION_PLANNING_NODE_HPP_

#include "tyr/planning/node.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/planning/state_view.hpp"

#include <string>

namespace tyr::serialization
{

template<TaskKind T>
struct Serializer<planning::Node<T>>
{
    static std::string name() { return std::string(T::name) + "Node"; }

    template<class Archive>
    static void save(Archive& archive, const planning::Node<T>& node)
    {
        archive.field("state", node.get_state());
        archive.field("metric", node.get_metric());
    }
};

template<TaskKind T>
struct Serializer<planning::LabeledNode<T>>
{
    static std::string name() { return std::string(T::name) + "LabeledNode"; }

    template<class Archive>
    static void save(Archive& archive, const planning::LabeledNode<T>& node)
    {
        archive.field("label", node.label);
        archive.field("node", node.node);
    }
};

}

#endif
