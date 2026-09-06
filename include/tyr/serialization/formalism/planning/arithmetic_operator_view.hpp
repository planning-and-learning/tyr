#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_ARITHMETIC_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_ARITHMETIC_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/arithmetic_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/planning/binary_operator_view.hpp"
#include "tyr/serialization/formalism/planning/multi_operator_view.hpp"
#include "tyr/serialization/formalism/planning/unary_operator_view.hpp"

namespace tyr::serialization
{

template<TaskKind T>
struct TypeName<formalism::planning::ArithmeticOperatorView<T>>
{
    static std::string get() { return std::string(std::same_as<T, GroundTag> ? T::name : "") + "ArithmeticOperator"; }
};

template<TaskKind T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const formalism::planning::ArithmeticOperatorView<T>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        ar.variant(value.get_variant());
    });
}

}

#endif
