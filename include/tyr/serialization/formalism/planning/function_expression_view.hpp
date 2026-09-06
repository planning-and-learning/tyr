#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_EXPRESSION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_EXPRESSION_VIEW_HPP_

#include "tyr/formalism/planning/function_expression_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/arithmetic_operator_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<>
struct Serializer<::tyr::formalism::planning::FunctionExpressionView>
{
    static std::string name() { return "FunctionExpression"; }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::FunctionExpressionView& value)
    {
        ar.variant(value.get_variant());
    }
};

}

#endif
