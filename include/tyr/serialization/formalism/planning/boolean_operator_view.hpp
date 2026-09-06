#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_BOOLEAN_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_BOOLEAN_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/boolean_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/binary_operator_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<TaskKind T>
struct Serializer<formalism::planning::BooleanOperatorView<T>>
{
    static std::string name() { return std::string(std::same_as<T, GroundTag> ? T::name : "") + "BooleanOperator"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::BooleanOperatorView<T>& value)
    {
        ar.variant(value.get_variant());
    }
};

}

#endif
