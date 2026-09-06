#ifndef TYR_SERIALIZATION_FORMALISM_FUNCTION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_FUNCTION_VIEW_HPP_

#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"

namespace tyr::serialization
{

template<formalism::FactKind T>
struct TypeName<formalism::planning::FunctionView<T>>
{
    static std::string get() { return std::string(T::name) + "Function"; }
};

template<formalism::FactKind T>
void tag_invoke(boost::json::value_from_tag, boost::json::value& result, const formalism::planning::FunctionView<T>& value, Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("name", value.get_name());
                             ar.field("arity", value.get_arity());
                         });
}

}

#endif
