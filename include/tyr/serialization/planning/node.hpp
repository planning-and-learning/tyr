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

template<::tyr::TaskKind T>
void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::planning::Node<T>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("state", value.get_state());
                             ar.field("metric", value.get_metric());
                         });
}

template<::tyr::TaskKind T>
struct TypeName<::tyr::planning::LabeledNode<T>>
{
    static std::string get() { return std::string(T::name) + "LabeledNode"; }
};

template<::tyr::TaskKind T>
void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::planning::LabeledNode<T>& value, Dictionaries* dictionaries)
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
