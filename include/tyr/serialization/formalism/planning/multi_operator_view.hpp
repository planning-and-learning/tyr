#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_MULTI_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_MULTI_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/multi_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<TaskKind T>
struct Serializer<formalism::planning::MultiOperatorView<T>>
{
    static std::string name() { return std::string(std::same_as<T, GroundTag> ? T::name : "") + "MultiOperator"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::MultiOperatorView<T>& value)
    {
        ar.field("operator", value.get_operator());
        ar.field("args", value.get_args());
    }
};

}

#endif
