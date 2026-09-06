#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_TERM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_TERM_VIEW_HPP_

#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/term_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<::tyr::formalism::FactKind T>
struct Serializer<::tyr::formalism::planning::FunctionTermView<T>>
{
    static std::string name() { return std::string(T::name) + "FunctionTerm"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::FunctionTermView<T>& value)
    {
        ar.field("function", value.get_function());
        ar.field("terms", value.get_terms());
    }
};

}

#endif
