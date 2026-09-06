#ifndef TYR_SERIALIZATION_FORMALISM_VARIABLE_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_VARIABLE_VIEW_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/serialization/dictionaries.hpp"

namespace tyr::serialization
{

template<>
struct TypeName<formalism::planning::VariableView>
{
    static std::string get() { return "Variable"; }
};

inline void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const formalism::planning::VariableView& value, Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar) { ar.field("name", value.get_name()); });
}

}

#endif
