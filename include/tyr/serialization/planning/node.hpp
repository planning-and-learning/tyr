#ifndef TYR_SERIALIZATION_PLANNING_NODE_HPP_
#define TYR_SERIALIZATION_PLANNING_NODE_HPP_

#include "tyr/planning/node.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/planning/state_view.hpp"

#include <string>

namespace tyr::serialization
{

template<TaskKind T>
struct TypeName<planning::Node<T>>
{
    static std::string get() { return std::string(T::name) + "Node"; }
};

template<TaskKind T>
void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const planning::Node<T>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("state", value.get_state());
                             ar.field("metric", value.get_metric());
                         });
}

template<TaskKind T>
struct TypeName<planning::LabeledNode<T>>
{
    static std::string get() { return std::string(T::name) + "LabeledNode"; }
};

template<TaskKind T>
void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const planning::LabeledNode<T>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("label", value.label);
                             ar.field("node", value.node);
                         });
}

}

#endif
