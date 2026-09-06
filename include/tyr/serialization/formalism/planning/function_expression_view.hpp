#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_EXPRESSION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_EXPRESSION_VIEW_HPP_

#include "tyr/formalism/planning/function_expression_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/dictionaries.hpp"
#include "tyr/serialization/formalism/planning/arithmetic_operator_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"

namespace tyr::serialization
{

template<>
struct TypeName<ygg::float_t>
{
    static std::string get() { return "constant"; }
};

template<TaskKind T>
struct TypeName<formalism::planning::FunctionExpressionView<T>>
{
    static std::string get() { return std::string(std::same_as<T, GroundTag> ? T::name : "") + "FunctionExpression"; }
};

template<TaskKind T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const formalism::planning::FunctionExpressionView<T>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar)
    {
        ar.variant(value.get_variant());
    });
}

}

#endif
