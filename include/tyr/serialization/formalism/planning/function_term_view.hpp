#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_TERM_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_TERM_VIEW_HPP_

#include "tyr/formalism/planning/function_term_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/binding_view.hpp"
#include "tyr/serialization/formalism/function_view.hpp"
#include "tyr/serialization/formalism/term_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<TaskKind T, formalism::FactKind F>
struct Serializer<formalism::planning::FunctionTermView<T, F>>
{
    static std::string name() { return std::string(F::name) + (std::same_as<T, GroundTag> ? T::name : "") + "FunctionTerm"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::FunctionTermView<T, F>& value)
    {
        if constexpr (std::same_as<T, LiftedTag>)
        {
            ar.field("function", value.get_function());
            ar.field("terms", value.get_terms());
        }
        else
        {
            ar.field("binding", value.get_row());
        }
    }
};

}

#endif
