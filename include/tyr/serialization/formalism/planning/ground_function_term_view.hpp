#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_FUNCTION_TERM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_FUNCTION_TERM_VIEW_HPP_

#include "tyr/formalism/planning/ground_function_term_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::formalism::FactKind T>
struct Serializer<::tyr::formalism::planning::GroundFunctionTermView<T>>
{
    static std::string name() { return std::string(T::name) + "GroundFunctionTerm"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::GroundFunctionTermView<T>& value)
    {
        ar.field("binding", value.get_row());
    }
};

}

#endif
