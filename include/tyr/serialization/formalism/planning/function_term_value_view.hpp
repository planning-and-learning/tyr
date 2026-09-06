#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_TERM_VALUE_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_TERM_VALUE_VIEW_HPP_

#include "tyr/formalism/planning/function_term_value_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<formalism::FactKind F>
struct Serializer<formalism::planning::FunctionTermValueView<GroundTag, F>>
{
    static std::string name() { return std::string(F::name) + "GroundFunctionTermValue"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::FunctionTermValueView<GroundTag, F>& value)
    {
        ar.field("fterm", value.get_fterm());
        ar.field("value", value.get_value());
    }
};

}

#endif
