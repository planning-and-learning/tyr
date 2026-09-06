#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_BINARY_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_BINARY_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/binary_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"
#include "tyr/serialization/serializer.hpp"

namespace tyr::serialization
{

template<TaskKind T, formalism::BinaryOperatorKind O>
struct Serializer<formalism::planning::BinaryOperatorView<T, O>>
{
    static std::string name()
    {
        return std::string(std::same_as<T, GroundTag> ? T::name : "") + "Binary"
               + (std::same_as<O, formalism::ArithmeticOperatorKind> ? "ArithmeticOperator" : "BooleanOperator");
    }

    template<class Archive>
    static void save(Archive& ar, const formalism::planning::BinaryOperatorView<T, O>& value)
    {
        ar.field("operator", value.get_operator());
        ar.field("lhs", value.get_lhs());
        ar.field("rhs", value.get_rhs());
    }
};

}

#endif
