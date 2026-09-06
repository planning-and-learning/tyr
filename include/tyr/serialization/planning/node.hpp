#ifndef TYR_SERIALIZATION_PLANNING_NODE_HPP_
#define TYR_SERIALIZATION_PLANNING_NODE_HPP_

#include "tyr/planning/node.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/planning/state_view.hpp"

#include <string>

namespace tyr::serialization
{

template<TaskKind Kind>
struct Serializer<planning::Node<Kind>>
{
    static std::string name() { return std::same_as<Kind, GroundTag> ? "GroundNode" : "LiftedNode"; }

    template<class Archive>
    static void save(Archive& archive, const planning::Node<Kind>& node)
    {
        archive.field("state", node.get_state());
        archive.field("metric", node.get_metric());
    }
};

template<TaskKind Kind>
struct Serializer<planning::LabeledNode<Kind>>
{
    static std::string name() { return std::same_as<Kind, GroundTag> ? "GroundLabeledNode" : "LiftedLabeledNode"; }

    template<class Archive>
    static void save(Archive& archive, const planning::LabeledNode<Kind>& node)
    {
        archive.field("label", node.label);
        archive.field("node", node.node);
    }
};

}

#endif
