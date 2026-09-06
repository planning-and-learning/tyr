#ifndef TYR_SERIALIZATION_FORMALISM_OBJECT_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_OBJECT_VIEW_HPP_

#include "tyr/formalism/object_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<>
struct TypeName<::tyr::formalism::planning::ObjectView>
{
    static std::string get() { return "Object"; }
};

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const ::tyr::formalism::planning::ObjectView& value, Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar) { ar.field("name", value.get_name()); });
}

}

#endif
