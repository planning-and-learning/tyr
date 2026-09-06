#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_EXPRESSION_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_FUNCTION_EXPRESSION_VIEW_HPP_

#include "tyr/formalism/planning/function_expression_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/arithmetic_operator_view.hpp"
#include "tyr/serialization/formalism/planning/function_term_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<>
struct TypeName<ygg::float_t>
{
    static std::string get() { return "constant"; }
};

template<::tyr::TaskKind T>
struct TypeName<::tyr::formalism::planning::FunctionExpressionView<T>>
{
    static std::string get() { return std::string(std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "FunctionExpression"; }
};

template<::tyr::TaskKind T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const ::tyr::formalism::planning::FunctionExpressionView<T>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar) { ar.variant(value.get_variant()); });
}

}

#endif
