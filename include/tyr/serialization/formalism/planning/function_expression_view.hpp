#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_EXPRESSION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_EXPRESSION_VIEW_HPP_

#include "tyr/formalism/planning/function_expression_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/arithmetic_operator_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<TaskKind T>
struct Serializer<formalism::planning::FunctionExpressionView<T>>
{
    static std::string name() { return std::string(std::same_as<T, GroundTag> ? T::name : "") + "FunctionExpression"; }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::FunctionExpressionView<T>& value)
    {
        ar.variant(value.get_variant());
    }
};

}

#endif
