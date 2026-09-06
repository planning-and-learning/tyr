#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_BINARY_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_BINARY_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/binary_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"

namespace tyr::serialization
{

template<TaskKind T, formalism::BinaryOperatorKind O>
struct TypeName<formalism::planning::BinaryOperatorView<T, O>>
{
    static std::string get()
    {
        return std::string(std::same_as<T, GroundTag> ? T::name : "") + "Binary"
               + (std::same_as<O, formalism::ArithmeticOperatorKind> ? "ArithmeticOperator" : "BooleanOperator");
    }
};

template<TaskKind T, formalism::BinaryOperatorKind O>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const formalism::planning::BinaryOperatorView<T, O>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        ar.field("operator", value.get_operator());
        ar.field("lhs", value.get_lhs());
        ar.field("rhs", value.get_rhs());
    });
}

}

#endif
