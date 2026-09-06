#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_UNARY_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_UNARY_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/unary_operator_view.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::TaskKind T>
struct TypeName<::tyr::formalism::planning::UnaryOperatorView<T>>
{
    static std::string get() { return std::string(std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "UnaryOperator"; }
};

template<::tyr::TaskKind T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const ::tyr::formalism::planning::UnaryOperatorView<T>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result,
                         value,
                         [&](auto& ar)
                         {
                             ar.field("operator", value.get_operator());
                             ar.field("arg", value.get_arg());
                         });
}

}

#endif
