#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_ARITHMETIC_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_ARITHMETIC_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/arithmetic_operator_view.hpp"
#include "tyr/formalism/planning/repository.hpp"
#include "tyr/serialization/formalism/planning/binary_operator_view.hpp"
#include "tyr/serialization/formalism/planning/multi_operator_view.hpp"
#include "tyr/serialization/formalism/planning/unary_operator_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<::tyr::TaskKind T>
struct TypeName<::tyr::formalism::planning::ArithmeticOperatorView<T>>
{
    static std::string get() { return std::string(std::same_as<T, ::tyr::GroundTag> ? T::name : "") + "ArithmeticOperator"; }
};

template<::tyr::TaskKind T>
void tag_invoke(boost::json::value_from_tag,
                boost::json::value& result,
                const ::tyr::formalism::planning::ArithmeticOperatorView<T>& value,
                Dictionaries* dictionaries)
{
    dictionaries->object(result, value, [&](auto& ar) { ar.variant(value.get_variant()); });
}

}

#endif
