#ifndef TYR_SERIALIZATION_FORMALISM_VARIABLE_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_VARIABLE_VIEW_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/variable_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<formalism::planning::VariableView>
{
    static std::string name() { return "Variable"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::VariableView& value)
    {
        ar.field("name", value.get_name());
    }
};

}

#endif
