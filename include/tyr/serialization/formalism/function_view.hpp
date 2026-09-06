#ifndef TYR_SERIALIZATION_FORMALISM_FUNCTION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_FUNCTION_VIEW_HPP_

#include "tyr/formalism/function_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<formalism::FactKind T>
struct Serializer<formalism::planning::FunctionView<T>>
{
    static std::string name() { return std::string(T::name) + "Function"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::FunctionView<T>& value)
    {
        ar.field("name", value.get_name());
        ar.field("arity", value.get_arity());
    }
};

}

#endif
