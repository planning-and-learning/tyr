#ifndef TYR_SERIALIZATION_FORMALISM_OBJECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_OBJECT_VIEW_HPP_

#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"

namespace tyr::serialization
{

template<>
struct TypeName<formalism::planning::ObjectView>
{
    static std::string get() { return "Object"; }
};

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const formalism::planning::ObjectView& value, Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar) { ar.field("name", value.get_name()); });
}

}

#endif
