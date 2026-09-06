#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_UNARY_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_UNARY_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/unary_operator_view.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<typename T>
struct Serializer<::tyr::formalism::planning::UnaryOperatorView<T>>
{
    static std::string name()
    {
        return std::string(std::same_as<T, ygg::Data<::tyr::formalism::planning::FunctionExpression<::tyr::GroundTag>>> ? "Ground" : "") + "UnaryOperator";
    }

    template<class Archive>
    static void save(Archive& ar, const ::tyr::formalism::planning::UnaryOperatorView<T>& value)
    {
        ar.field("operator", value.get_operator());
        ar.field("arg", value.get_arg());
    }
};

}

#endif
