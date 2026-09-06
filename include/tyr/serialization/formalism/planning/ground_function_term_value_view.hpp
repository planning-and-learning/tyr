#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_FUNCTION_TERM_VALUE_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_FUNCTION_TERM_VALUE_VIEW_HPP_

#include "tyr/formalism/planning/ground_function_term_value_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/ground_function_term_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::formalism::FactKind T>
struct Serializer<::tyr::formalism::planning::GroundFunctionTermValueView<T>>
{
    static std::string name() { return std::string(T::name) + "GroundFunctionTermValue"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::GroundFunctionTermValueView<T>& value)
    {
        ar.field("fterm", value.get_fterm());
        ar.field("value", value.get_value());
    }
};

}

#endif
