#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_TERM_VALUE_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_TERM_VALUE_VIEW_HPP_

#include "tyr/formalism/planning/function_term_value_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::formalism::FactKind F>
struct TypeName<::tyr::formalism::planning::FunctionTermValueView<::tyr::GroundTag, F>>
{
    static std::string get() { return std::string(F::name) + "GroundFunctionTermValue"; }
};

template<::tyr::formalism::FactKind F>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const ::tyr::formalism::planning::FunctionTermValueView<::tyr::GroundTag, F>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("function_term", value.get_fterm());
                             ar.field("value", value.get_value());
                         });
}

}

#endif
