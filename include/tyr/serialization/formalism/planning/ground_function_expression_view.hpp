#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_FUNCTION_EXPRESSION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_GROUND_FUNCTION_EXPRESSION_VIEW_HPP_

#include "tyr/formalism/planning/ground_function_expression_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/arithmetic_operator_view.hpp"
#include "tyr/serialization/formalism/planning/ground_function_term_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::GroundFunctionExpressionView>
{
    static std::string name() { return "GroundFunctionExpression"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::GroundFunctionExpressionView& value)
    {
        ar.variant(value.get_variant());
    }
};

}

#endif
