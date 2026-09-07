#ifndef TYR_SERIALIZATION_FORMALISM_PLANNING_UNARY_OPERATOR_VIEW_HPP_
#define TYR_SERIALIZATION_FORMALISM_PLANNING_UNARY_OPERATOR_VIEW_HPP_

#include "tyr/formalism/planning/repository.hpp"
#include "tyr/formalism/planning/unary_operator_view.hpp"
#include "tyr/serialization/formalism/enums.hpp"
#include "tyr/serialization/formalism/planning/function_expression_view.hpp"
#include "yggdrasil/serialization/dictionaries.hpp"

namespace ygg::serialization
{

template<class Archive, ::tyr::TaskKind T>
void describe_fields(Archive& ar, std::type_identity<::tyr::formalism::planning::UnaryOperatorView<T>>)
{
    ar.field("operator", [](const auto& value) -> decltype(auto) { return (value.get_operator()); });
    ar.field("arg", [](const auto& value) -> decltype(auto) { return (value.get_arg()); });
}

}

#endif
